/**********************************************************
* file: io_event.h
* brief: IO event model, such as select/epoll/iocp
* 
* author: qk
* email: 
* date: 2018-08
* modify date: 
**********************************************************/

#ifndef _IO_EVENT_H_
#define _IO_EVENT_H_

#include "net_error.h"

//recvf buf max len
#define NET_BUF_MAX_LEN (1024*5)

#ifdef __cplusplus
extern "C" {
#endif

//notify type
enum EEV_NOTIFY_TYPE {
	ENT_ACCEPT=0,
	ENT_DATA,
	ENT_CLOSE
};
//notify data
struct event_notify_data {
	enum EEV_NOTIFY_TYPE type;
	char *data;
	int len;
};
struct io_handle;
//event notify callback
//return: if nd->type==EIO_ENT_DATA, processed data len, other type ignore
typedef unsigned int (*pfunc_event_notify)(const struct io_handle *handle, unsigned short channel, struct event_notify_data *nd);


/**********************************************************
 * brief: init io_event env
 * input: size, the max number of monitored socket
 *        pf, event notify callback function
 *
 * return: -1 error, 0 ok
 *********************************************************/
int io_event_init(int size, pfunc_event_notify pf);

/**********************************************************
 * brief: create tcp server/connection and monitor it
 * input: ip, host ip addr or null/empty string
 *        port, host port
 *        channel, id value for different communication
 *
 * return: NULL error, other ok
 *********************************************************/
struct io_handle* io_event_create_tcp(const char *ip, unsigned short port, unsigned short channel);

/**********************************************************
 * brief: create udp server/connection and monitor it
 * input: ip, host ip addr or null/empty string
 *        port, host port
 *        channel, id value for different communication
 *
 * return: NULL error, other ok
 *********************************************************/
struct io_handle* io_event_create_udp(const char *ip, unsigned short port, unsigned short channel);

/**********************************************************
 * brief: close io_handle and not to monitor it
 * input: hd, io_handle
 *
 * return: None
 *********************************************************/
void io_event_close_handle(struct io_handle *hd);

/**********************************************************
 * brief: send data on io_handle hd
 * input: hd, io handle
 *        data, will send data
 *        len, data len
 *
 * return: -1 error, >=0 actually len send data
 *********************************************************/
int io_event_send_data(struct io_handle *hd, const char *data, int len);

/**********************************************************
 * brief: start thread for monitor io event
 * input: None
 *
 * return: -1 error, 0 ok
 *********************************************************/
int io_event_run();

/**********************************************************
 * brief: stop thread that monitoring io event
 * input: None
 *
 * return: None
 *********************************************************/
void io_event_stop();

/**********************************************************
 * brief: release io_event env
 * input: None
 *
 * return: None
 *********************************************************/
void io_event_release();

#ifdef __cplusplus
}
#endif

#endif//_IO_EVENT_H_

