/**********************************************************
* file: thread.h
* brief: thread independ on os platform
* 
* author: qk
* email: 
* date: 2018-08
* modify date: 
**********************************************************/

#ifndef _THREAD_H_
#define _THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

//thread callback function for outside
typedef void (*pfunc_thread_proc)(void *arg);

/**********************************************************
 * brief: create thread
 * input: pf, thread call function
 *        arg, param for thread
 *
 * return: NULL error, other ok
 *********************************************************/
struct thread_t* thread_create(pfunc_thread_proc pf, void *arg);

/**********************************************************
 * brief: wait thread exit
 * input: th, thread handle from thread_create function
 *
 * return: None
 *********************************************************/
void thread_join(struct thread_t **th);


#ifdef __cplusplus
}
#endif

#endif //_THREAD_H_

