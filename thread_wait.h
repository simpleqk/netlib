/**********************************************************
* file: thread_wait.h
* brief: thread synchro by signal
* 
* author: qk
* email: 
* date: 2018-07-26
* modify date: 2018-08
**********************************************************/

#ifndef _THREAD_WAIT_H_
#define _THREAD_WAIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
 * brief: create thread wait object
 * input: none
 * return: NULL error, other ok
 *********************************************************/
struct thread_wait_t* thread_wait_create();

/**********************************************************
 * brief: wait until have signal
 * input: tw, thread wait object
 * return: 0 ok, -1 error
 *********************************************************/
int thread_wait(struct thread_wait_t *tw);

/**********************************************************
 * brief: wait until have signal or timeout
 * input: tw, thread wait object
 *        milliseconds time out value
 * return: 0 ok, -1 error, 1 timeout
 *********************************************************/
int thread_wait_time(struct thread_wait_t *tw, unsigned int milliseconds);

/**********************************************************
 * brief: notify signal
 * input: tw, thread wait object
 * return: 0 ok, -1 error
 *********************************************************/
int thread_wait_signal(struct thread_wait_t *tw);

/**********************************************************
 * brief: destroy threa wait object
 * input: tw, cond_wait object
 * return: None
 *********************************************************/
void thread_wait_destroy(struct thread_wait_t *tw);

#ifdef __cplusplus
}
#endif

#endif //_THREAD_WAIT_H_

