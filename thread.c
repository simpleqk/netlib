#include "thread.h"
#include "mem_pool.h"
#include "log.h"
#include "net_error.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #include <pthread.h>
#endif //_WIN32

struct thread_t {
#ifdef _WIN32
	HANDLE hThread;
	HANDLE hEventExit;
	ULONG  tid;
#else
	pthread_t tid;
#endif //_WIN32
	pfunc_thread_proc pf;
	void *arg;
};

//thread callback function
#ifdef _WIN32
DWORD WINAPI ThreadProc(LPVOID param) {
#else
void* thread_proc(void *param) {
#endif //_WIN32
	struct thread_t *th = (struct thread_t*)param;
	if(th) {
		th->pf(th->arg);
	}

#ifdef _WIN32
	SetEvent(th->hEventExit);
#endif //_WIN32
	return 0;
}

//typedef void (*pfunc_thread_proc)(void *arg);

struct thread_t* thread_create(pfunc_thread_proc pf, void *arg)
{
	struct thread_t *th;
	
	if(NULL==pf) {
		return NULL;
	}

	th = (struct thread_t*)mem_pool_malloc(sizeof(struct thread_t));
	if(NULL==th) {
		return NULL;
	} else {
		th->pf = pf;
		th->arg = arg;
	}
#ifdef _WIN32
	th->hEventExit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(NULL==th->hEventExit) {
		return NULL;
	}
	th->hThread = CreateThread(NULL, 0, ThreadProc, (void*)th, 0, &th->tid);
	if(NULL==th->hThread) {
		//error
		CloseHandle(th->hEventExit);
		mem_pool_free(th);
		return NULL;
	}
#else
	if(0!=pthread_create(&th->tid, NULL, thread_proc, (void*)th)) {
		//error
		mem_pool_free(th);
		return NULL;
	}
#endif //_WIN32

	return th;
}

void thread_join(struct thread_t **th)
{
	if(NULL==th || NULL==*th) {
		return;
	}

#ifdef _WIN32
	WaitForSingleObject((*th)->hEventExit, INFINITE);
	CloseHandle((*th)->hEventExit);
	CloseHandle((*th)->hThread);
#else
	pthread_join((*th)->tid, NULL);
#endif //_WIN32

	mem_pool_free(*th);
	*th = NULL;
}

