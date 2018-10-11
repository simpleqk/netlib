/**********************************************************
* file: io_event_api.h
* brief: IO event model, such as select/epoll/iocp
* 
* author: qk
* email: 
* date: 2018-08
* modify date: 
**********************************************************/

#ifndef _IO_EVENT_API_H_
#define _IO_EVENT_API_H_

#include "socket_api.h"
#include "net_error.h"

#ifdef __cplusplus
extern "C" {
#endif

struct io_handle {
	SOCKET s;
	char param[0];
};
struct io_event;
//io event notify callback
typedef void (*pfunc_io_event_notify)(struct io_event *ie, const struct io_handle *handle);


/**********************************************************
 * brief: create io_event object
 * input: size, the max number of monitored object
 *
 * return: NULL error, other ok
 *********************************************************/
struct io_event* io_event_create(int size);

/**********************************************************
 * brief: loop for monitoring io event, can be run in multi-threads
 * input: ie, io event object
 *        pf, function pointer for notify outsid the io event
 *
 * return: 0 ok, -1 error
 *********************************************************/
int io_event_loop(struct io_event *ie, pfunc_io_event_notify pf);

/**********************************************************
 * brief: stop loop that monitoring io event
 * input: ie, io event object
 *
 * return: None
 *********************************************************/
void io_event_stop_loop(struct io_event *ie);

/**********************************************************
 * brief: add monitor object
 * input: ie, io event object
 *        hd, io handle
 *
 * return: 0 ok, -1 error
 *********************************************************/
int io_event_add(struct io_event *ie, struct io_handle *hd);

/**********************************************************
 * brief: delete monitor object
 * input: ie, io event object
 *        hd, io handle
 *
 * return: 0 ok, -1 error
 *********************************************************/
int io_event_del(struct io_event *ie, struct io_handle *hd);

/**********************************************************
 * brief: destroy io_event object
 * input: ie, io event object
 *
 * return: None
 *********************************************************/
void io_event_destroy(struct io_event *ie);

#ifdef __cplusplus
}
#endif

#endif//_IO_EVENT_API_H_

