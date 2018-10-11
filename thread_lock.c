#include "thread_lock.h"
#include <stdlib.h>
#include "log.h"

#ifdef _WIN32
  #include<windows.h>
#else
  #include<pthread.h>
#endif //_WIN32

//align 4 algorithm
#define ALIGN4(n) ((n+3)&~3)

enum ETLOCK_TYPE {
	ETL_CRITICAL=0,
	ETL_MUTEX
};

struct tlock_t {
	enum ETLOCK_TYPE type;
};
//critical section
struct tlock_critical_t {
#ifdef _WIN32
	CRITICAL_SECTION cs;
#else
  #ifdef _ANDROID
	pthread_mutex_t mutex;
  #else
	pthread_spinlock_t spin;
  #endif //_ANDROID
#endif //_WIN32
};
//mutex
struct tlock_mutex_t {
#ifdef _WIN32
	HANDLE hMutex;
#else 
	pthread_mutex_t mutex;
#endif //_WIN32
};

static inline int lock_critical_section(struct tlock_t *tl);
static inline int trylock_critical_section(struct tlock_t *tl);
static inline int unlock_critical_section(struct tlock_t *tl);

static inline int lock_mutex(struct tlock_t *tl);
static inline int trylock_mutex(struct tlock_t *tl);
static inline int unlock_mutex(struct tlock_t *tl);

static inline void destroy_clock(struct tlock_t *tl);
static inline void destroy_mlock(struct tlock_t *tl);


struct tlock_t* lock_create_critical_section()
{
	struct tlock_t *tl;
	struct tlock_critical_t *tc;
	unsigned int size = sizeof(struct tlock_t)+sizeof(struct tlock_critical_t);

	size = ALIGN4(size);
	tl = (struct tlock_t*)malloc(size);
	if(tl) {
		tl->type = ETL_CRITICAL;
		tc = (struct tlock_critical_t*)(tl+1);
#ifdef _WIN32
		InitializeCriticalSection(&tc->cs);
#else
  #ifdef _ANDROID
		if(0!=pthread_mutex_init(&tc->mutex, NULL)) {
			free(tl);
			tl = NULL;
		}
  #else
		if(0!=pthread_spin_init(&tc->spin, PTHREAD_PROCESS_PRIVATE)) {
			free(tl);
			tl = NULL;
		}
  #endif //_ANDROID
#endif //_WIN32
	}

	return tl;
}

struct tlock_t* lock_create_mutex()
{
	struct tlock_t *tl;
	struct tlock_mutex_t *tm;
	unsigned int size = sizeof(struct tlock_t)+sizeof(struct tlock_mutex_t);

	size =  ALIGN4(size);
	tl = (struct tlock_t*)malloc(size);
	if(tl) {
		tl->type = ETL_MUTEX;
		tm = (struct tlock_mutex_t*)(tl+1);
#ifdef _WIN32
		tm->hMutex = CreateMutex(NULL, FALSE/*initially not owned*/, NULL);
		if(NULL==tm->hMutex) {
			free(tl);
			tl = NULL;
		}
#else
		if(0!=pthread_mutex_init(&tm->mutex, NULL)) {
			free(tl);
			tl = NULL;
		}
#endif //_WIN32
	}

	return tl;
}

int lock_lock(struct tlock_t* tl)
{
	switch(tl->type) {
		case ETL_CRITICAL:
			return lock_critical_section(tl);
			break;
		case ETL_MUTEX:
			return lock_mutex(tl);
			break;
		default:
			LOG_WARN("[thread_lock] lock_lock failed, unknow type=%d", tl->type);
			break;
	}

	return -1;
}

int lock_trylock(struct tlock_t* tl)
{
	switch(tl->type) {
		case ETL_CRITICAL:
			return trylock_critical_section(tl);
			break;
		case ETL_MUTEX:
			return trylock_mutex(tl);
			break;
		default:
			LOG_WARN("[thread_lock] lock_trylock failed, unknow type=%d", tl->type);
			break;
	}

	return -1;
}

int lock_unlock(struct tlock_t* tl)
{
	switch(tl->type) {
		case ETL_CRITICAL:
			return unlock_critical_section(tl);
			break;
		case ETL_MUTEX:
			return unlock_mutex(tl);
			break;
		default:
			LOG_WARN("[thread_lock] lock_unlock failed, unknow type=%d", tl->type);
			break;
	}

	return -1;
}

void lock_destroy(struct tlock_t *tl)
{
	switch(tl->type) {
		case ETL_CRITICAL:
			destroy_clock(tl);
			break;
		case ETL_MUTEX:
			destroy_mlock(tl);
			break;
		default:
			LOG_WARN("[thread_lock] lock_destroy failed, unknow type=%d", tl->type);
			break;
	}
}

static inline int lock_critical_section(struct tlock_t *tl)
{
	struct tlock_critical_t *tc = (struct tlock_critical_t*)(tl+1);
#ifdef _WIN32
	EnterCriticalSection(&tc->cs); return 0;
#else
  #ifdef _ANDROID
	return (0==pthread_mutex_lock(&tc->mutex)) ? (0) : (-1);
  #else
	return (0==pthread_spin_lock(&tc->spin)) ? (0) : (-1);
  #endif //_ANDROID
#endif //_WIN32
}

static inline int trylock_critical_section(struct tlock_t *tl)
{
	struct tlock_critical_t *tc = (struct tlock_critical_t*)(tl+1);
#ifdef _WIN32
	return (0==TryEnterCriticalSection(&tc->cs)) ? (-1) : (0);
#else
  #ifdef _ANDROID
	return (0==pthread_mutex_trylock(&tc->mutex)) ? (0) : (-1);
  #else
	return (0==pthread_spin_trylock(&tc->spin)) ? (0) : (-1);
  #endif //_ANDROID
#endif //_WIN32
}

static inline int unlock_critical_section(struct tlock_t *tl)
{
	struct tlock_critical_t *tc = (struct tlock_critical_t*)(tl+1);
#ifdef _WIN32
	LeaveCriticalSection(&tc->cs); return 0;
#else
  #ifdef _ANDROID
	return (0==pthread_mutex_unlock(&tc->mutex)) ? (0) : (-1);
  #else
	return (0==pthread_spin_unlock(&tc->spin)) ? (0) : (-1);
  #endif //_ANDROID
#endif //_WIN32
}

static inline int lock_mutex(struct tlock_t *tl)
{
	struct tlock_mutex_t *tc = (struct tlock_mutex_t*)(tl+1);
#ifdef _WIN32
	return (WAIT_FAILED==WaitForSingleObject(tc->hMutex, INFINITE)) ? (-1) : (0);
#else
	return (0==pthread_mutex_lock(&tc->mutex)) ? (0) : (-1);
#endif //_WIN32
}

static inline int trylock_mutex(struct tlock_t *tl)
{
	struct tlock_mutex_t *tc = (struct tlock_mutex_t*)(tl+1);
#ifdef _WIN32
	return (WAIT_OBJECT_0==WaitForSingleObject(tc->hMutex, 1)) ? (0) : (-1);
#else
	return (0==pthread_mutex_trylock(&tc->mutex)) ? (0) : (-1);
#endif //_WIN32
}

static inline int unlock_mutex(struct tlock_t *tl)
{
	struct tlock_mutex_t *tc = (struct tlock_mutex_t*)(tl+1);
#ifdef _WIN32
	return (0==ReleaseMutex(tc->hMutex)) ? (-1) : (0);
#else
	return (0==pthread_mutex_unlock(&tc->mutex)) ? (0) : (-1);
#endif //_WIN32
}

static inline void destroy_clock(struct tlock_t *tl)
{
	struct tlock_critical_t *tc = (struct tlock_critical_t*)(tl+1);
#ifdef _WIN32
	DeleteCriticalSection(&tc->cs);
#else
  #ifdef _ANDROID
	pthread_mutex_destroy(&tc->mutex);
  #else
	pthread_spin_destroy(&tc->spin);
  #endif //_ANDROID
#endif //_WIN32
	free(tl);
}

static inline void destroy_mlock(struct tlock_t *tl)
{
	struct tlock_mutex_t *tm = (struct tlock_mutex_t*)(tl+1);
#ifdef _WIN32
	CloseHandle(&tm->hMutex);
#else
	pthread_mutex_destroy(&tm->mutex);
#endif //_WIN32
	free(tl);
}

