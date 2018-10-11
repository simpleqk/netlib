/**********************************************************
* file: thread_lock.h
* brief: thread lock implement 
*        include critical section and mutex
* 
* author: qk
* email: 
* date: 2018-09
* modify date:
**********************************************************/

#ifndef _THREAD_LOCK_H_
#define _THREAD_LOCK_H_

//create criticalSection lock
struct tlock_t* lock_create_critical_section();

//create mutex lock
struct tlock_t* lock_create_mutex();

//lock
int lock_lock(struct tlock_t *tl);
int lock_trylock(struct tlock_t *tl);

//unlock
int lock_unlock(struct tlock_t *tl);

//destroy lock
void lock_destroy(struct tlock_t *tl);

#endif //_THREAD_LOCK_H_

