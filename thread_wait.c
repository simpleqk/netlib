#include "thread_wait.h"
#include <stdlib.h>
#include "atomic.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #include <errno.h>
  #include <pthread.h>
  #include <time.h>
#endif //_WIN32

struct thread_wait_t {
#ifdef _WIN32
	HANDLE hEvent;
#else
	pthread_mutex_t mutex;
	pthread_cond_t cond;
#endif //_WIN32
	long signal;
};

static inline char* mem_malloc(unsigned int size) {
	size = (size +3)&(~3);
	return malloc(size);
}
static inline void mem_free(void *mem) {
	free(mem);
}

struct thread_wait_t* thread_wait_create()
{
	struct thread_wait_t* tw = (struct thread_wait_t*)mem_malloc(sizeof(struct thread_wait_t));
	if(tw) {
#ifdef _WIN32
		tw->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if(NULL==tw->hEvent) {
			mem_free(tw);
			tw = NULL;
		}
#else
		pthread_mutex_init(&tw->mutex, NULL);
		pthread_cond_init(&tw->cond, NULL);
#endif //_WIN32
		tw->signal = 0;
	}

	return tw;
}

int thread_wait(struct thread_wait_t *tw)
{
	if(tw) {
		if(atomic_get(&tw->signal)) {
			atomic_set(&tw->signal, 0);
			return 0;
		}
#ifdef _WIN32
		WaitForSingleObject(tw->hEvent, INFINITE);
#else
		pthread_mutex_lock(&tw->mutex);
		pthread_cond_wait(&tw->cond, &tw->mutex);
		pthread_mutex_unlock(&tw->mutex);
#endif //_WIN32
		atomic_set(&tw->signal, 0);
		return 0;
	}

	return -1;
}

int thread_wait_time(struct thread_wait_t *tw, unsigned int milliseconds)
{
	int ret = -1;
#ifdef _WIN32
#else
	struct timespec ts;
#endif //_WIN32

	if(tw) {
		if(atomic_get(&tw->signal)) {
			atomic_set(&tw->signal, 0);
			return 0;
		}
#ifdef _WIN32
		switch(WaitForSingleObject(tw->hEvent, milliseconds)) {
			case WAIT_OBJECT_0:
				ret = 0; //have signal, ok
				break;
			case WAIT_TIMEOUT: //timeout
				ret = 1;
				break;
			default: //error
				ret = -1;
				break;
		}
#else
		pthread_mutex_lock(&tw->mutex);
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += milliseconds/1000;
		ts.tv_nsec += milliseconds%1000 * 1000;
		switch(pthread_cond_timedwait(&tw->cond, &tw->mutex, &ts)) {
			case 0: //have signal, ok
				ret = 0;
				break;
			case ETIMEDOUT: //timeout
				ret = 1;
				break;
			default: //error
				ret = -1;
				break;
		}
		pthread_mutex_unlock(&tw->mutex);
#endif //_WIN32
		atomic_set(&tw->signal, 0);
	}

	return ret;
}

int thread_wait_signal(struct thread_wait_t *tw)
{
	if(tw) {
		atomic_set(&tw->signal, 1);
#ifdef _WIN32
		SetEvent(tw->hEvent);
#else
		pthread_mutex_lock(&tw->mutex);
		pthread_cond_signal(&tw->cond);
		pthread_mutex_unlock(&tw->mutex);
#endif //_WIN32
		return 0;
	}

	return -1;
}

void thread_wait_destroy(struct thread_wait_t *tw)
{
	if(tw) {
#ifdef _WIN32
		CloseHandle(tw->hEvent);
#else
		pthread_mutex_destroy(&tw->mutex);
		pthread_cond_destroy(&tw->cond);
#endif //_WIN32
		mem_free(tw);
	}
}

