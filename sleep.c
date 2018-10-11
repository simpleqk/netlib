#include "sleep.h"

#ifdef _WIN32
  #include <windows.h>
  #include <winsock2.h>
#else
  #include <sys/time.h>
  #include <sys/types.h>
  #include <unistd.h>
  #include <errno.h>
#endif //_WIN32

#ifdef _WIN32
#else
static inline int do_sleep(struct timeval *tv);
#endif //_WIN32

int sleeps(unsigned long seconds)
{
#ifdef _WIN32
#else
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = 0;

	return do_sleep(&tv);
#endif //_WIN32
}

int sleepm(unsigned long milliseconds)
{
#ifdef _WIN32
#else
	struct timeval tv;
	tv.tv_sec = milliseconds/1000;
	tv.tv_usec = (milliseconds%1000) * 1000;

	return do_sleep(&tv);
#endif //_WIN32
}

int sleepu(unsigned long microseconds)
{
#ifdef _WIN32
	LARGE_INTEGER freq = {0};
	LARGE_INTEGER start = {0};
	LARGE_INTEGER end = {0};

	if(0==QueryPerformanceFrequency(&freq)) {
		return ;
	}
	if(0==QueryPerformanceCounter(&start)) {
		return ;
	}
	while(1) {
		QueryPerformanceCounter(&end);
		((end.QuadPart - start.QuadPart)*1000000)/freq.QuadPart;
	}
#else
	struct timeval tv;
	tv.tv_sec = microseconds/1000000;
	tv.tv_usec = microseconds%1000000;

	return do_sleep(&tv);
#endif //_WIN32
}

#ifdef _WIN32
#else
static inline int do_sleep(struct timeval *tv)
{
	int err;
	do {
		err=select(0, NULL, NULL, NULL, tv);
	}while(err<0 && EINTR==errno);

	return (-1==err) ? (-1) : (0);
}
#endif //_WIN32

