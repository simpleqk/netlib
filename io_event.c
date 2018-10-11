#include "io_event.h"
#include "io_event_api.h"
#include "socket_api.h"
#include "mem_pool.h"
#include "hash_map.h"
#include "log.h"
#include "thread.h"
#include "thread_lock.h"
#include "typedef.h"
#include "net_error.h"
#include <string.h>

__thread int net_errno;

//type
enum ESOCKET_TYPE {
	EST_UNKNOWN = 0,
	EST_TCP_SERVER,
	EST_UDP_SERVER,
	EST_TCP_CLIENT,
	EST_UDP_CLIENT
};

//struct io_handle derived class
struct io_event_data {
	SOCKET s; //must first

	enum ESOCKET_TYPE type;
	unsigned short channel;
	unsigned short buf_data_len;
	//option udp only
	//struct sockaddr peer_addr[0];
	//option udp/tcp-client only
	char buf[0]; //NET_BUF_MAX_LEN
};
//#define IODT_OFFSET_TCP_BUF(ed) (ed->buf)
//#define IODT_OFFSET_UDP_BUF(ed) (ed->buf+sizeof(struct sockaddr))

//for hash_map custom function
static inline int hash_map_isvalid_val(long val) {
	return (0==val) ? (0) : (1);
}
static inline void hash_map_free_val(long val) {
	if(val) {
		socket_close( ((struct io_event_data*)val)->s );
		mem_pool_free((struct io_event_data*)val); 
	}
}

struct hash_map *g_mem_hash_map; //<SOCKET, struct io_event_data*>
struct io_event *g_io_event;
pfunc_event_notify g_nt_func;
struct thread_t *g_thread_handle;

static struct tlock_t *g_tlock;
#define LOCK() lock_lock(g_tlock);
#define UNLOCK() lock_unlock(g_tlock);


static int io_event_join_handle(struct io_handle *hd);

static void thread_run(void *arg);
static void io_event_notify_handle(struct io_event *ie, const struct io_handle *handle);
static void io_event_accept_client(struct io_event *ie, struct io_event_data *ed, pfunc_event_notify pf);
static void io_event_read_udp(struct io_event *ie, struct io_event_data *ed, pfunc_event_notify pf);
static void io_event_read_tcp(struct io_event *ie, struct io_event_data *ed, pfunc_event_notify pf);


int io_event_init(int size, pfunc_event_notify pf)
{
	struct hash_map_func hmf;

	if(size<=0 || NULL==pf) {
		LOG_WARN("[io_event] init failed, param is invalid.");
		return -1;
	}
	
	if(g_io_event) {
		LOG_WARN("[io_event] init failed, have inited.");
		return -1;
	}

	//init mem pool
	if(-1==mem_pool_init(10)) {
		LOG_WARN("[io_event] init failed, init mem_pool failed.");
		return -1;
	}

	//thread lock
	g_tlock = lock_create_critical_section();
	if(NULL==g_tlock) {
		LOG_WARN("[io_event] init failed, create thread lock failed.");
		return -1;
	}

#ifdef _WIN32
	if(-1==socket_init_env()) {
		LOG_WARN("[io_event] init failed, init socket system envirenment failed.");
		return -1;
	}
#endif //_WIN32
	hash_map_inner_hmf(&hmf, EFI_LONG_LONG);
	hmf.isvalid_val = hash_map_isvalid_val;
	hmf.free_val = hash_map_free_val;
	g_mem_hash_map = hash_map_create(size/2, &hmf);
	if(NULL==g_mem_hash_map) {
		LOG_WARN("[io_event] init failed, hash map create failed.");
		return -1;
	}

	g_io_event = io_event_create(size);
	if(NULL==g_io_event) {
		hash_map_destroy(g_mem_hash_map);
		LOG_WARN("[io_event] init failed, event create failed.");
		return -1;
	}

	g_nt_func = pf;

	return 0;
}

struct io_handle* io_event_create_tcp(const char *ip, unsigned short port, unsigned short channel)
{
	SOCKET s;
	enum ESOCKET_TYPE type;
	struct io_event_data *ed=NULL;

	if(NULL==g_io_event) {
		LOG_WARN("[io_event] create tcp failed, not init.");
		return NULL;
	}

	s = socket_create_tcp(ip, port);
	if(INVALID_SOCKET==s) {
		LOG_WARN("[io_event] create tcp failed, create socket failed.");
		return NULL;
	}

	if(NULL==ip || '\0'==*ip) {
		//tcp server
		type = EST_TCP_SERVER;
		ed = (struct io_event_data*)mem_pool_malloc(sizeof(struct io_event_data));
	} else {
		type = EST_TCP_CLIENT;
		ed = (struct io_event_data*)mem_pool_malloc(sizeof(struct io_event_data)+NET_BUF_MAX_LEN);
	}

	if(ed) {
		ed->s = s;
		ed->type = type;
		ed->channel = channel;
		ed->buf_data_len = 0;

		//add to io_event
		if(-1==io_event_join_handle((struct io_handle*)ed)) {
			LOG_WARN("[io_event] create tcp failed, join handle to io_event failed.");
			return NULL;
		}
	}

	return (struct io_handle*)ed;
}

struct io_handle* io_event_create_udp(const char *ip, unsigned short port, unsigned short channel)
{
	SOCKET s;
	enum ESOCKET_TYPE type;
	struct io_event_data *ed=NULL;

	if(NULL==g_io_event) {
		LOG_WARN("[io_event] create udp failed, not init.");
		return NULL;
	}

	s = socket_create_udp(ip, port);
	if(INVALID_SOCKET==s) {
		return NULL;
	}

	if(NULL==ip || '\0'==*ip) {
		//udp server
		type = EST_UDP_SERVER;
		ed = (struct io_event_data*)mem_pool_malloc(sizeof(struct io_event_data)+/*sizeof(struct sockaddr)+*/NET_BUF_MAX_LEN);
	} else {
		type = EST_UDP_CLIENT;
		ed = (struct io_event_data*)mem_pool_malloc(sizeof(struct io_event_data)+/*sizeof(struct sockaddr)+*/NET_BUF_MAX_LEN);
	}

	if(ed) {
		ed->s = s;
		ed->type = type;
		ed->channel = channel;
		ed->buf_data_len = 0;

		//add to io_event
		if(-1==io_event_join_handle((struct io_handle*)ed)) {
			LOG_WARN("[io_event] create udp failed, join handle to io_event failed.");
			return NULL;
		}
	}

	return (struct io_handle*)ed;
}

void io_event_close_handle(struct io_handle *hd)
{
	long s;
	if(g_io_event && hd) {
		s = (long)hd->s;
		LOCK();
		io_event_del(g_io_event, hd);
		hash_map_del(g_mem_hash_map, s);
		UNLOCK();
		LOG_DEBUG("[io_event] removed socket=%ld from io_event.", s);
	}
}

int io_event_send_data(struct io_handle *hd, const char *data, int len)
{
	struct io_event_data *ed = (struct io_event_data*)hd;

	if(NULL==ed || NULL==data || 0==len) {
		LOG_WARN("[io_event] send data failed, param is invalid.");
		return -1;
	}

	switch(ed->type) {
	case EST_UNKNOWN:
		LOG_WARN("[io_event] send data failed, SOCKET type is unknown.");
		return -1;
		break;
	case EST_TCP_SERVER:
		LOG_WARN("[io_event] send data failed, SOCKET type is listenning socket which cannot send data on it.");
		return -1;
		break;
	case EST_UDP_SERVER:
		return socket_send_udp(ed->s, data, len);
		break;
	case EST_TCP_CLIENT:
		return socket_send_tcp(ed->s, data, len);
		break;
	case EST_UDP_CLIENT:
		return socket_send_udp(ed->s, data, len);
		break;
	default:
		return -1;
		break;
	}
}

int io_event_run()
{
	if(g_thread_handle) {
		return -1;
	}

	if(NULL == (g_thread_handle = thread_create(thread_run, NULL))) {
		return -1;
	}

	return 0;
}

void io_event_stop()
{
	io_event_stop_loop(g_io_event);
	thread_join(&g_thread_handle);
}

void io_event_release()
{
	if(g_io_event) {
		io_event_stop();
		io_event_destroy(g_io_event);
		g_io_event = NULL;
		hash_map_destroy(g_mem_hash_map);
		g_mem_hash_map = NULL;
		lock_destroy(g_tlock);
		g_tlock = NULL;
		mem_pool_release();
	}
#ifdef _WIN32
	socket_release_env();
#endif //_WIN32
}

static int io_event_join_handle(struct io_handle *hd)
{
	////struct io_event_data *ed = (struct io_event_data*)hd;

	//thread lock
	LOCK();

	//add mem pointer to hash_map
	if(-1==hash_map_add(g_mem_hash_map, (long)hd->s, (long)hd)) {
		mem_pool_free(hd);
		LOG_WARN("[io_event] join io_handle to io_event, add data to hash_map failed.");
		UNLOCK();
		return -1;
	}
	//add to io_event object
	if(-1==io_event_add(g_io_event, hd)) {
		hash_map_del(g_mem_hash_map, (long)hd->s);
		LOG_WARN("[io_event] join io_handle to io_event, add data to io_event failed.");
		UNLOCK();
		return -1;
	}

	UNLOCK();

	LOG_DEBUG("[io_event] join socket=%ld to io_event ok.", (long)hd->s);

	return 0;
}

static void thread_run(void *arg)
{
	//use variable only for compiler
	(void)arg;
	io_event_loop(g_io_event, io_event_notify_handle);
}

static void io_event_notify_handle(struct io_event *ie, const struct io_handle *handle)
{
	struct io_event_data *ed = (struct io_event_data*)handle;

	switch(ed->type) {
		case EST_TCP_SERVER://accept
			LOG_DEBUG("[io_event] have event on socket=%ld, type=TCP-S.", (long)ed->s);
			io_event_accept_client(ie, ed, g_nt_func);
			break;
		case EST_UDP_SERVER://read
			LOG_DEBUG("[io_event] have event on socket=%ld, type=UDP-S.", (long)ed->s);
			io_event_read_udp(ie, ed, g_nt_func);
			break;
		case EST_TCP_CLIENT://read
			LOG_DEBUG("[io_event] have event on socket=%ld, type=TCP-C.", (long)ed->s);
			io_event_read_tcp(ie, ed, g_nt_func);
			break;
		case EST_UDP_CLIENT://read
			LOG_DEBUG("[io_event] have event on socket=%ld, type=UDP-C.", (long)ed->s);
			io_event_read_udp(ie, ed, g_nt_func);
			break;
		default:
		//case EST_UNKNOWN:
			LOG_WARN("[io_event] handle event failed, socket=%ld type is unknow", (long)ed->s);
			break;
	}
}

static void io_event_accept_client(struct io_event *ie, struct io_event_data *ed, pfunc_event_notify pf)
{
	SOCKET c;
	struct io_event_data *newed;
	struct sockaddr_in addr;
	struct event_notify_data nd;

	//use variable only for compiler
	(void)ie;

	if(0==socket_accept_client(ed->s, &c, (struct sockaddr*)&addr)) {
		LOG_DEBUG("[io_event] handle event and accept new tcp client=%d [%s:%d] successfully", 
				c, socket_convert_val2ip(addr.sin_addr.s_addr), addr.sin_port);

		//add to io monitor
		newed = (struct io_event_data*)mem_pool_malloc(sizeof(struct io_event_data));
		if(newed) {
			newed->s = c;
			newed->type = EST_TCP_CLIENT;
			newed->channel = ed->channel;
			newed->buf_data_len = 0;

			//add to io_event
			if(-1==io_event_join_handle((struct io_handle*)ed)) {
				LOG_WARN("[io_event] handle event and accept new client failed at listening socket=%d, join handle to io_event failed.", ed->s);
				return ;
			}
			
			//notify outside
			nd.type = ENT_ACCEPT;
			nd.data = NULL;
			nd.len = 0;
			pf((struct io_handle*)ed, ed->channel, &nd);
		}
		else {
			LOG_WARN("[io_event] handle event and accept new client failed at listening socket=%d, create io_handle failed.", ed->s);
		}
	}
	else {
		LOG_WARN("[io_event] handle event and accept new client failed at listening socket=%d.", ed->s);
	}
}

static void io_event_read_udp(struct io_event *ie, struct io_event_data *ed, pfunc_event_notify pf)
{
	int recv_len;
	unsigned int proc_len;
	int left_len = NET_BUF_MAX_LEN - ed->buf_data_len;
	struct event_notify_data nd;

	//use variable only for compiler
	(void)ie;

	if(left_len <= 0) {
		LOG_WARN("[io_event] handle event and there is no space to receive udp data at client=%d.", ed->s);
		return ;
	}

	recv_len = socket_recv_udp(ed->s, /*&ed->peer_addr,*/ ed->buf+ed->buf_data_len, left_len);
	if(recv_len>0) {
		LOG_DEBUG("[io_event] recv data len=%d from socket=%ld, type=UDP-C.", recv_len, (long)ed->s);
		ed->buf_data_len += recv_len;
		//notify outside
		nd.type = ENT_DATA;
		nd.data = ed->buf;
		nd.len = ed->buf_data_len;
		proc_len = pf((struct io_handle*)ed, ed->channel, &nd);
		if(proc_len>0 && proc_len<=ed->buf_data_len) {
			memmove(ed->buf, ed->buf+proc_len, ed->buf_data_len-proc_len);
			ed->buf_data_len -= proc_len;
		} else {
			LOG_WARN("[io_event] handle event and read udp data len=%d, but proc_len=%d is invalid", recv_len, proc_len);
		}
	}
	else if(0==recv_len) {
		//closed
		nd.type = ENT_CLOSE;
		nd.data = NULL;
		nd.len = 0;
		pf((struct io_handle*)ed, ed->channel, &nd);
		io_event_close_handle((struct io_handle*)ed);
	}
	else {
		LOG_WARN("[io_event] handle event and read udp client=%d data failed.", ed->s);
	}
}

static void io_event_read_tcp(struct io_event *ie, struct io_event_data *ed, pfunc_event_notify pf)
{
	int recv_len;
	unsigned int proc_len;
	int left_len = NET_BUF_MAX_LEN - ed->buf_data_len;
	struct event_notify_data nd;

	//use variable only for compiler
	(void)ie;

	if(left_len <= 0) {
		LOG_WARN("[io_event] handle event and there is no space to receive tcp data at client=%d.", ed->s);
		return ;
	}
	
	recv_len = socket_recv_tcp(ed->s, ed->buf+ed->buf_data_len, NET_BUF_MAX_LEN-ed->buf_data_len);
	if(recv_len>0) {
		LOG_DEBUG("[io_event] recv data len=%d from socket=%ld, type=TCP-C.", recv_len, (long)ed->s);
		ed->buf_data_len += recv_len;
		//notify outside
		nd.type = ENT_DATA;
		nd.data = ed->buf;
		nd.len = ed->buf_data_len;
		proc_len = pf((struct io_handle*)ed, ed->channel, &nd);
		if(proc_len>0 && proc_len<=ed->buf_data_len) {
			memmove(ed->buf, ed->buf+proc_len, ed->buf_data_len-proc_len);
			ed->buf_data_len -= proc_len;
		} else {
			LOG_WARN("[io_event] handle event and read tcp data len=%d, but proc_len=%d is invalid", recv_len, proc_len);
		}
	}
	else if(0==recv_len) {
		//closed
		nd.type = ENT_CLOSE;
		nd.data = NULL;
		nd.len = 0;
		pf((struct io_handle*)ed, ed->channel, &nd);
		io_event_close_handle((struct io_handle*)ed);
	}
	else {
		LOG_WARN("[io_event] handle event and read tcp client=%d data failed.", ed->s);
	}
}

