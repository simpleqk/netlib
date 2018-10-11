#include "io_event_api.h"
#include "mem_pool.h"
#include "log.h"
#include "net_error.h"
#include "typedef.h"

#ifdef _WIN32
  #include <Windows.h>
#else
  #include <errno.h>
  #include <unistd.h>
  #include <linux/types.h>
  #include <sys/epoll.h>
  #ifndef EPOLLONESHOT
    /*for NDK-buidl*/
    #ifdef __CHECK_POLL
       typedef unsigned __bitwise __poll_t;
    #else
       typedef unsigned __poll_t;
    #endif
    /* Set the One Shot behaviour for the target file descriptor */
    #define EPOLLONESHOT (__force __poll_t)(1U << 30)
  #endif
#endif //_WIN32

//io event object define
//count, current actived io count
//size, total io count
//handle, io handle
struct io_event {
	int count;
	int size;
	int stop;
	long handle;
};

struct io_event* io_event_create(int size)
{
	struct io_event *ie;
#ifdef _WIN32
	HANDLE hIOCP;
#else
	int efd;
#endif //_WIN32

	if(size <= 0) {
		LOG_WARN("[io_event_api] io_event_create failed, size is 0.");
		return NULL;
	}
	
	ie = (struct io_event*)mem_pool_malloc(sizeof(struct io_event));
	if(NULL==ie) {
		LOG_WARN("[io_event_api] io_event_create failed, mem_pool_malloc failed.");
		return NULL;
	} else {
		ie->count = 0;
		ie->size = size;
		ie->stop = 0;
	}

#ifdef _WIN32
	//HANDLE WINAPI CreateIoCompletionPort(
	//	_In_     HANDLE    FileHandle,
	//	_In_opt_ HANDLE    ExistingCompletionPort,
	//	_In_     ULONG_PTR CompletionKey,
	//	_In_     DWORD     NumberOfConcurrentThreads
	//);
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(NULL==hIOCP) {
		mem_pool_free(ie);
		LOG_WARN("[io_event_api] io_event_create failed, crate iocp failed.");
		return NULL;
	} else {
		ie->handle = (long)hIOCP;
	}
#else
	//count is not the maximum size of the backing store
	//Nowadays, size is ignored, Since Linux 2.6.8, the size argument is unused.
	efd = epoll_create(size);
	if(-1==efd) {
		mem_pool_free(ie);
		LOG_WARN("[io_event_api] io_event_create failed, crate epoll failed.");
		return NULL;
	} 
	else {
		ie->handle = (long)efd;
	}
#endif //_WIN32

	return ie;
}

int io_event_loop(struct io_event *ie, pfunc_io_event_notify pf)
{
	struct io_handle *hd;
#ifdef _WIN32
	DWORD bytes;
	LPOVERLAPPED pol;
#else
	#define MAX_EVENTS (20)
	struct epoll_event ev, evs[MAX_EVENTS];
	int nfds, i;
#endif //_WIN32

	if(NULL==ie || NULL==pf) {
		LOG_WARN("[io_event_api] event loop failed, param is invalid.");
		return -1;
	}

	//loop for monitoring
	while(1) {
		if(ie->stop) {
			break;
		}
#ifdef _WIN32
		/*BOOL WINAPI GetQueuedCompletionStatus(
		_In_  HANDLE       CompletionPort,
		_Out_ LPDWORD      lpNumberOfBytes,
		_Out_ PULONG_PTR   lpCompletionKey,
		_Out_ LPOVERLAPPED *lpOverlapped,
		_In_  DWORD        dwMilliseconds
		);*/
		if(TRUE==GetQueuedCompletionStatus((HANDLE)ie->handle, 
			(LPDWORD)&bytes, (LPDWORD)&hd,
			&pol,
			INFINITE)) {
			if(0==bytes) {
				//client be closed
				pf(ie, hd, bytes, pol);
			} else {
				//handle event
				pf(ie, hd, bytes, pol);
			}
		} else {
			if(ERROR_ABANDONED_WAIT_0 == GetLastError()) {
				// the completion port handle was closed while the call is outstanding
				// pol is null
				pf(ie, hd, bytes, pol);
			} else {
				LOG_WARN("[io_event_api] GetQueuedCompletionStatus failed.");
			}
		}
#else
		nfds = epoll_wait(ie->handle, evs, /*maxevents*/MAX_EVENTS, /*timeout-milliseconds*/400);
		if(-1==nfds) {
			if(EINTR==errno) {
				//interrupted by highest process such as gdb
				continue;
			}
			//error
			LOG_WARN("[io_event_api] epoll_wait failed, errno=%d", errno);
			ie->stop = 1;
		}

		for(i=0;i<nfds;++i) {
			hd = (struct io_handle*)evs[i].data.ptr;
			ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
			ev.data.ptr = hd;
			pf(ie, hd);
			if(-1==epoll_ctl(ie->handle, EPOLL_CTL_MOD, hd->s, &ev)) {
				if(EBADF==errno) {
					//mayne hd->s have been closed in pf function
				} else {
					LOG_WARN("[io_event_api] after handling event at socket=%d, do EPOLL_CTL_MOD failed.", hd->s);
				}
			}
		}
#endif //_WIN32
	}

	LOG_WARN("[io_event_api] event loop exit.");

	return 0;
}

void io_event_stop_loop(struct io_event *ie)
{
#ifdef _WIN32
	OVERLAPPED ol = {0, 0, 0, 0, NULL};
#endif //_WIN32

	if(ie) {
		ie->stop = 1;
#ifdef _WIN32
		PostQueuedCompletionStatus((HANDLE)ie->handle,
			NULL, NULL, NULL, &ol);
#endif //_WIN32
	}
}

int io_event_add(struct io_event *ie, struct io_handle *hd)
{
	int ret;
#ifdef _WIN32
#else
	struct epoll_event ev;
#endif //_WIN32

	if(NULL==ie || NULL==hd) {
		LOG_WARN("[io_event_api] event add failed, param is invalid.");
		return -1;
	}

	if(ie->count >= ie->size) {
		LOG_WARN("[io_event_api] event add failed, io handle count is up to limit=%d.", ie->size);
		return -1;
	}

#ifdef _WIN32
	/*HANDLE WINAPI CreateIoCompletionPort(
	_In_     HANDLE    FileHandle,
	_In_opt_ HANDLE    ExistingCompletionPort,
	_In_     ULONG_PTR CompletionKey,
	_In_     DWORD     NumberOfConcurrentThreads
	);*/
	//The handle s must be to an object that supports overlapped I/O.
	ret = (NULL==CreateIoCompletionPort((HANDLE)hd->s, (HANDLE)ie->handle, (ULONG_PTR)hd, 0)) ? (-1) : (0);
#else
	/*typedef union epoll_data {
               void        *ptr;
               int          fd;
               __uint32_t   u32;
               __uint64_t   u64;
           } epoll_data_t;

           struct epoll_event {
               __uint32_t   events;      * Epoll events *
               epoll_data_t data;        * User data variable *
           };
	*/
	//EPOLLIN, notify event when readable
	//EPOLLET, notify event when fd state changed, but the state not include that have data in buffer not be handled
	//EPOLLONESHOT (since Linux 2.6.2), after an event is pulled out with epoll_wait(2) the associated file descriptor 
	// is internally disabled and no other events will be  reported. The user must call epoll_ctl() with EPOLL_CTL_MOD 
	// to re-arm the file descriptor with a new event mask
	ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	ev.data.ptr = hd;

	//successful, epoll_ctl() returns zero.
	//When an error occurs, epoll_ctl() returns -1
	ret = epoll_ctl(ie->handle, EPOLL_CTL_ADD, hd->s, &ev);
#endif //_WIN32

	if(0==ret) {
		ie->count++;
	}
	return ret;
}

int io_event_del(struct io_event *ie, struct io_handle *hd)
{
	int ret;
	struct epoll_event ev;

	if(NULL==ie || NULL==hd) {
		LOG_WARN("[io_event_api] event del failed, param is invalid.");
		return -1;
	}

#ifdef _WIN32
	if(ie->count > 0) {
		ret = 0;
	}
#else
	//The event is ignored and can be NULL (but see BUGS below).
	ret = epoll_ctl(ie->handle, EPOLL_CTL_DEL, hd->s, &ev);
#endif //_WIN32

	if(0==ret) {
		ie->count--;
	}
	return ret;
}

void io_event_destroy(struct io_event *ie)
{
	if(ie) {
#ifdef _WIN32
		CloseHandle((HANDLE)ie->handle);
#else
		close((int)ie->handle);
#endif //_WIN32
		mem_pool_free(ie);
	}
}

