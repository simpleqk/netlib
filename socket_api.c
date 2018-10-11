#include "socket_api.h"
#include <string.h>
#include "log.h"
#include "net_error.h"

#ifdef _WIN32
  #include <Ws2tcpip.h>
  #include <Iphlpapi.h>    /*Iphlpapi.lib*/
#else
  /*fcntl*/
  #include <unistd.h>
  #include <fcntl.h>
  /*ioctl*/
  #include <sys/ioctl.h>
  /*getaddrinfo*/
  #include <netdb.h>
  /*struct ifreq*/
  #include <net/if.h>
  #include <errno.h>
  #include <sys/select.h>
#endif //_WIN32

//listening queue length
#define NET_LISTEN_QUEUE_LEN (10)

static SOCKET socket_create_server(unsigned short port, int flag);
static SOCKET socket_connect_server(const char *ip, unsigned short port, int flag);

//return: -2 timeout, -1 error, 0-ok
static int socket_check_connect(SOCKET s);

int socket_init_env()
{
#ifdef _WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		LOG_WARN("[socket_api] socket init failed, WSAStartup failed with error: %d", err);
		return -1;
	}
	/* Confirm that the WinSock DLL supports 2.2./
	/ Note that if the DLL supports versions greater    /
	/ than 2.2 in addition to 2.2, it will still return /
	/ 2.2 in wVersion since that is the version we      /
	/ requested.                                       */
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		LOG_WARN("[socket_api] Could not find a usable version of Winsock.dll");
		WSACleanup();
		return 1;
	}
#endif //_WIN32

	return 0;
}

void socket_release_env()
{
#ifdef _WIN32
	WSACleanup();
#endif //_WIN32
}

SOCKET socket_create_tcp(const char *ip, unsigned short port)
{
	SOCKET s;
	if(NULL==ip || '\0'==*ip) {
		s = socket_create_server(port, SOCK_STREAM);
		//windows: 0 ok, SOCKET_ERROR (-1) error
		//linux: 0 ok, -1 error
		if(0==listen(s, NET_LISTEN_QUEUE_LEN)) {
			return s;
		} else {
			socket_close(s);
			LOG_WARN("[socket_api] socket create tcp failed, listen at s=%d error.", s);
			return -1;
		}
	}
	else {
		return socket_connect_server(ip, port, SOCK_STREAM);
	}
}

SOCKET socket_create_udp(const char *ip, unsigned short port)
{
	if(NULL==ip || '\0'==*ip) {
		return socket_create_server(port, SOCK_DGRAM);
	}
	else {
		return socket_connect_server(ip, port, SOCK_DGRAM);
	}
}

static SOCKET socket_create_server(unsigned short port, int flag)
{
	SOCKET s;
	struct sockaddr_in addr;

	/*struct sockaddr {
		sa_family_t sin_family;    //address family
		char        sa_data[14];   //14byte, include dest addr and port
	};
	struct sockaddr_in {
		sa_family_t    sin_family;  //address family
		uint16_t       sin_port;    //16bit TCP/UDP port
		struct in_addr sin_addr;    //32bit IP addr
		char           sin_zero[8]; //reserve
	};
	typedef uint32_t in_addr_t;
	struct in_addr {
		in_addr_t s_addr;
	};
	*/

	//If a value of 0 is specified, the caller does not wish to specify a protocol 
	//and the service provider will choose the protocol to use.
	s = socket(AF_INET, flag, 0);
	if(INVALID_SOCKET==s) {
		net_errno = NET_ERROR_MALLOC_SOCKET;
		return -1;
	}

	//set nonblock
	if(-1==socket_set_nonblock(s)) {
		socket_close(s);
		net_errno = NET_ERROR_SET_NONBLOCK;
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//bind
	if(0 != bind(s, (struct sockaddr*)&addr, sizeof(struct sockaddr))) {
		socket_close(s);
		LOG_WARN("[socket_api] socket create server failed, bind socket s=%d error, errno=%d", s, errno);
		net_errno = NET_ERROR_BIND;
		return -1;
	}

	return s;
}

static SOCKET socket_connect_server(const char *ip, unsigned short port, int flag)
{
	int ret;
	unsigned int ipval;
	SOCKET s;
	struct sockaddr_in addr;

	if(NULL==ip || '\0'==*ip || 0==port) {
		net_errno = NET_ERROR_INVALID_PARAM;
		return -1;
	}

	ipval = socket_convert_ip2val(ip);

	s = socket(AF_INET, flag, 0);
	if(INVALID_SOCKET==s) {
		net_errno = NET_ERROR_MALLOC_SOCKET;
		return -1;
	}

	//set nonblock
	if(-1==socket_set_nonblock(s)) {
		socket_close(s);
		net_errno = NET_ERROR_SET_NONBLOCK;
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = ipval;

	//connect
	if(0 != connect(s, (struct sockaddr*)&addr, sizeof(struct sockaddr))) {
		ret = errno;
		if(EINPROGRESS/*115*/==errno) {
			//The socket is non-blocking and the connection cannot be completed immediately.
			if(0==(ret=socket_check_connect(s))) {
				return s;
			}
		}
		socket_close(s);
		if(-2==ret) {
			LOG_WARN("[socket_api] socket connect server timeout.");
		} else {
			LOG_WARN("[socket_api] socket connect server failed, errno=%d.", errno);
		}
		net_errno = NET_ERROR_CONNECT;
		return -1;
	}

	return s;
}

static int socket_check_connect(SOCKET s)
{
	int ret;
	fd_set rfds, wfds;
	struct timeval tv;
	int error;
	socklen_t len;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(s, &rfds);
	FD_SET(s, &wfds);

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	ret = select(s+1, &rfds, &wfds, NULL, &tv);
	switch(ret) {
		case -1: //error
			break;
		case 0: //time out
			ret = -2;
			break;
		default: //have event
			if(FD_ISSET(s, &rfds) || FD_ISSET(s, &wfds)) {
				len = sizeof(error);
				ret = getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &len);
				if(ret<0 || error) {
					//error
					ret = -1;
				} else {
					ret = 0;
				}
			}
			break;
	}

	return ret;
}

int socket_accept_client(SOCKET s, SOCKET *c, struct sockaddr *addr)
{
	socklen_t len;

	if(NULL==c || NULL==addr) {
		return -1;
	}

	len = sizeof(struct sockaddr);
	*c = accept(s, addr, &len);
	
	return (INVALID_SOCKET==*c) ? -1 : 0;
}

int socket_send_tcp(SOCKET s, const char *data, int len)
{
	int ret;
	int count=3;
	if(NULL==data || 0==len) {
		net_errno = NET_ERROR_INVALID_PARAM;
		return -1;
	}

#ifdef _WIN32
	ret = send(s, data, len, 0);
#else
	do {
		//MSG_NOSIGNAL (since Linux 2.2)
		ret = send(s, data, len, MSG_NOSIGNAL);
		if(ret < 0) {
			if(EAGAIN==errno || EWOULDBLOCK==errno) {
				//go on
			} else if(EPIPE==errno) {
				//The local end has been shut down on a connection oriented socket
				return -1;
			}
		}
		
		if(ret>=len) {
			break;
		}
	}while(--count);
#endif //_WIN32

	return ret;
}

int socket_recv_tcp(SOCKET s, char *buf, int len)
{
	if(NULL==buf || 0==len) {
		net_errno = NET_ERROR_INVALID_PARAM;
		return -1;
	}

	return recv(s, buf, len, 0);
}

int socket_send_udp(SOCKET s, /*struct sockaddr *peer_addr,*/const char *data, int len)
{
	//struct sockaddr detail, see function socket_create_server
	if(/*NULL==peer_addr ||*/ NULL==data || 0==len) {
		net_errno = NET_ERROR_INVALID_PARAM;
		return -1;
	}

#ifndef _WIN32
	return sendto(s, data, len, 0, /*peer_addr*/NULL, /*sizeof(struct sockaddr)*/0);
#else
	return sendto(s, data, len, MSG_NOSIGNAL, /*peer_addr*/NULL, /*sizeof(struct sockaddr)*/0);
#endif //_WIN32
}

int socket_recv_udp(SOCKET s, /*struct sockaddr *peer_addr,*/ char *buf, int len)
{
	int addr_len;

	if(/*NULL==peer_addr ||*/ NULL==buf || 0==len) {
		net_errno = NET_ERROR_INVALID_PARAM;
		return -1;
	}

	addr_len = sizeof(struct sockaddr);

	return recvfrom(s, buf, len, 0, /*peer_addr*/NULL, /*&addr_len*/0);
}

void socket_close(SOCKET s)
{
#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif //_WIN32
}

int socket_set_nonblock(SOCKET s)
{
	int flag;
#ifdef _WIN32
	/*Set the socket I/O mode: In this case FIONBIO
	enables or disables the blocking mode for the
	socket based on the numerical value of iMode.
	If iMode = 0, blocking is enabled;
	If iMode != 0, non-blocking mode is enabled.
	*/
	flag = 1;
	return (NO_ERROR == ioctlsocket(s, FIONBIO, &flag)) ? 0 : -1;
#else
	flag = fcntl(s, F_GETFL);
	flag |= O_NONBLOCK;

	return (0==fcntl(s, F_SETFL, flag)) ? 0 : -1;
#endif //_WIN32
}

int socket_get_local_addr(SOCKET s, struct sockaddr_in *addr)
{
	socklen_t addr_len = sizeof(struct sockaddr);
	//On success, zero is returned.  On error, -1 is returned
	return (addr) ? getsockname(s, (struct sockaddr*)addr, &addr_len) : -1;
}

int socket_get_peer_addr(SOCKET s, struct sockaddr_in *addr)
{
	socklen_t addr_len = sizeof(struct sockaddr);
	//On success, zero is returned.  On error, -1 is returned
	return (addr) ? getpeername(s, (struct sockaddr*)addr, &addr_len) : -1;
}

char* socket_get_host_ip(const char *host)
{
	int ret;
	struct addrinfo hints;
	struct addrinfo *res, *next;
	unsigned int ipvalue = 0;

	if(NULL==host || '\0'==*host) {
		return NULL;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; //0 any protocol
	hints.ai_flags = AI_PASSIVE; //for tcp
	
	if(0==(ret=getaddrinfo(host, NULL, &hints, &res))) {
		next = res;
		while(res) {
			next = res->ai_next;
			//struct sockaddr
			ipvalue = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
			freeaddrinfo(res);
			res = next;
		};

		return socket_convert_val2ip(ipvalue);
	} else {
		LOG_WARN("[socket_api] getaddrinfo(host=%s) failed errno=%d,%s", host, ret, gai_strerror(ret));
	}

	return NULL;
}

unsigned int socket_get_netcard_ip(const char *eth)
{
	SOCKET s;
	unsigned int ip=0;
	struct ifreq ifr;

	if(NULL==eth || '\0'==*eth) {
		return 0;
	}

	if( (s = socket(AF_INET, SOCK_STREAM, 0)) != -1) {
		memset(&ifr, 0, sizeof(struct ifreq));
		strncpy(ifr.ifr_name, eth, sizeof(ifr.ifr_name)-1);
		
		if(0==ioctl(s, SIOCGIFADDR, &ifr)) {
			//struct sockaddr, ifr.ifr_addr
			//equal struct sockaddr_in
			ip = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
		}
		socket_close(s);
	} else {
		LOG_WARN("[socket_api] socket failed errno=%d", errno);
	}

	return ip;
}

unsigned int socket_convert_ip2val(const char *ip)
{
	struct in_addr ia;

	if(NULL==ip || '\0'==*ip) {
		return -1;
	}

	if(0==inet_aton(ip, &ia)) {
		//returns non-zero if the address is valid, zero if not
		LOG_WARN("[socket_api] inet_aton(%s) failed", ip);
		return -1;
	}

	return ia.s_addr;
	//return inet_addr(ip).s_addr;
}

char* socket_convert_val2ip(unsigned int ipval)
{
	/*typedef uint32_t in_addr_t;
	struct in_addr {
		in_addr_t s_addr;
	};
	*/
	struct in_addr ia;
	ia.s_addr = ipval;

	//The string is returned in a statically allocated buffer, 
	// which subsequent calls will overwrite.
	return inet_ntoa(ia);
}

