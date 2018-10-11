/**********************************************************
* file: atomic.h
* brief: atomic operator
* 
* author: qk
* email: 
* date: 2018-09
* modify date: 
**********************************************************/

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

extern long atomic_add(long volatile *dst, long add);
extern long atomic_sub(long volatile *dst, long sub);
extern long atomic_set(long volatile *dst, long s);
extern long atomic_compare_set(long volatile *dst, long oldval, long s);
extern long atomic_get(long volatile *dst);

#endif //_ATOMIC_H_

