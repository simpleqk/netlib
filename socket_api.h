/**********************************************************
* file: socket_api.h
* brief: os socket api
* 
* author: qk
* email: 
* date: 2018-08
* modify date: 
**********************************************************/

#ifndef _SOCKET_API_H_
#define _SOCKET_API_H_

#ifdef _WIN32
  #include<winsock2.h>           /*Ws2_32.lib*/
  //#define SOCKET               /*typedef u_int SOCKET;*/
  //#define INVALID_SOCKET (SOCKET)(~0)
  //#define SOCKET_ERROR   (-1)
#else
  #include <sys/types.h>         /*See NOTES*/
  #include <sys/socket.h>
  #include <netinet/in.h>        /*inet_addr*/
  #include <arpa/inet.h>
  #define SOCKET int
  #define INVALID_SOCKET (-1)    /*create*/
  #define SOCKET_ERROR   (-1)    /*send*/
#endif //_WIN32

#include "net_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
 * brief: init socket system envirenment
 * input: None
 *
 * return: -1 error, 0 ok
 *********************************************************/
int socket_init_env();

/**********************************************************
 * brief: release socket envirenment
 * input: None
 *
 * return: None
 *********************************************************/
void socket_release_env();

/**********************************************************
 * brief: create/connect-to tcp model server
 * input: ip, ip v4 string, such as "xxx.xxx.xxx.xxx"
 *            if null, create tcp server that listen at port
 *        port, peer server port or listening port
 *
 * return: INVALID_SOCKET error, other ok
 *         NET_ERROR_MALLOC_SOCKET
 *         NET_ERROR_SET_NONBLOCK
 *         NET_ERROR_BIND
 *********************************************************/
SOCKET socket_create_tcp(const char *ip, unsigned short port);

/**********************************************************
 * brief: create/connect-to udp model server
 * input: ip, ip v4 string, such as "xxx.xxx.xxx.xxx"
 *            if null, create udp server that listen at port
 *        port, peer server port or listening port
 *
 * return: INVALID_SOCKET error, other value ok
 *********************************************************/
SOCKET socket_create_udp(const char *ip, unsigned short port);

/**********************************************************
 * brief: accept client from listenning socket s
 * input: s, listenning SOCKET
 *        c, client SOCKET returned
 *        addr, client addr
 *
 * return: -1 error, 0 ok
 *********************************************************/
int socket_accept_client(SOCKET s, SOCKET *c, struct sockaddr *addr);

/**********************************************************
 * brief: send data to tcp server
 * input: s, created SOCKET
 *        data, will be sent data
 *        len, data length
 *
 * return: SOCKET_ERROR error, >0 the length have sent
 *********************************************************/
int socket_send_tcp(SOCKET s, const char *data, int len);

/**********************************************************
 * brief: recv data to tcp server
 * input: s, created SOCKET
 *        buf, buffer for receiving data
 *        len, buf length
 *
 * return: SOCKET_ERROR error, >0 the length have received
 *         =0, if the connection has been gracefully closed
 *********************************************************/
int socket_recv_tcp(SOCKET s, char *buf, int len);

/**********************************************************
 * brief: send data to udp server
 * input: s, created SOCKET, have connected to server
 *        peer_addr, client addr send to
 *        data, will be sent data
 *        len, data length
 *
 * return: SOCKET_ERROR error, >0 the length have sent
 *********************************************************/
int socket_send_udp(SOCKET s, /*struct sockaddr *peer_addr,*/ const char *data, int len);

/**********************************************************
 * brief: recv data to udp server
 * input: s, created SOCKET
 *        peer_addr, peer addr recvfrom
 *        buf, buffer for receiving data
 *        len, buf length
 *
 * return: SOCKET_ERROR error, >0 the length have received
 *         =0, if the connection has been gracefully closed
 *********************************************************/
int socket_recv_udp(SOCKET s, /*struct sockaddr *peer_addr,*/ char *buf, int len);

/**********************************************************
 * brief: close socket
 * input: s, created SOCKET
 *
 * return: None
 *********************************************************/
void socket_close(SOCKET s);

/**********************************************************
 * brief: set socket flag with nonblock
 * input: s, created SOCKET
 *
 * return: 0 ok, -1 error
 *********************************************************/
int socket_set_nonblock(SOCKET s);

/**********************************************************
 * brief: get local socket addr relative to s
 * input: s, created SOCKET
 *        addr, buffer for storing the addr info
 *
 * return: 0 ok, -1 error
 *********************************************************/
int socket_get_local_addr(SOCKET s, struct sockaddr_in *addr);

/**********************************************************
 * brief: get peer socket addr relative to s
 * input: s, created SOCKET
 *        addr, buffer for storing the addr info
 *
 * return: 0 ok, -1 error
 *********************************************************/
int socket_get_peer_addr(SOCKET s, struct sockaddr_in *addr);

/**********************************************************
 * brief: get domain host ip string
 * input: host, domain string
 *
 * return: NULL error, other ok
 *********************************************************/
char* socket_get_host_ip(const char *host);

/**********************************************************
 * brief: get local netcard ip value
 * input: eth, netcard name
 *
 * return: 0 error, other ip value
 *********************************************************/
unsigned int socket_get_netcard_ip(const char *eth);

/**********************************************************
 * brief: convert ip string to integer
 * input: ip, ip string
 *
 * return: ip integer value
 *********************************************************/
unsigned int socket_convert_ip2val(const char *ip);

/**********************************************************
 * brief: convert ip integer value to string
 * input: ipval, ip integer value
 *
 * return: ip string
 *********************************************************/
char* socket_convert_val2ip(unsigned int ipval);

#ifdef __cplusplus
}
#endif

#endif//_SOCKET_API_H_

