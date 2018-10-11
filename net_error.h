/**********************************************************
* file: net_error.h
* brief: error value define
* 
* author: qk
* email: 
* date: 2018-07-21
* modify date: 
**********************************************************/

#ifndef _NETLITE_NET_ERROR_H_
#define _NETLITE_NET_ERROR_H_

extern __thread int net_errno;

#define NET_ERROR_OK                 (0)
#define NET_ERROR_IS_RUNNING         (-100)
#define NET_ERROR_FD_FULL            (-101)
#define NET_ERROR_FD_NOT_EXIST       (-102)
#define NET_ERROR_SET_NONBLOCK       (-103)
#define NET_ERROR_INVALID_PARAM      (-104)
#define NET_ERROR_MALLOC_SOCKET      (-105)
#define NET_ERROR_BIND               (-106)
#define NET_ERROR_LISTEN             (-107)
#define NET_ERROR_CONNECT            (-108)
#define NET_ERROR_SEND               (-109)
#define NET_ERROR_RECV               (-110)
#define NET_ERROR_WRITE_LISTEN_FD    (-111)
#define NET_ERROR_MALLOC_HANDLE      (-112)
#define NET_ERROR_CREATE_THREAD      (-113)
#define NET_ERROR_CREATE_MAP         (-114)
#define NET_ERROR_WSASTARTUP         (-115)
#define NET_ERROR_HAVE_INIT          (-116)
#define NET_ERROR_CREATE_IOEVENT     (-117)

#endif //_NETLITE_NET_ERROR_H_

