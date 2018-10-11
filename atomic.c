#include "atomic.h"

#ifdef _WIN32
  #include<windows.h>
#else
#endif //_WIN32

inline long atomic_add(long volatile *dst, long add)
{
#ifdef _WIN32
	return InterlockedExchangeAdd(dst, add);
#else
	return __sync_fetch_and_add(dst, add);
#endif //_WIN32
}

inline long atomic_sub(long volatile *dst, long sub)
{
#ifdef _WIN32
	return InterlockedExchangeAdd(dst, -sub);
#else
	return __sync_fetch_and_sub(dst, sub);
#endif //_WIN32
}

inline long atomic_set(long volatile *dst, long s)
{
#ifdef _WIN32
	return InterlockedExchange(dst, s);
#else
	return __sync_val_compare_and_swap(dst, *dst, s);
#endif //_WIN32
}

inline long atomic_compare_set(long volatile *dst, long oldval, long s)
{
#ifdef _WIN32
	return InterlockedCompareExchange(dst, s, oldval);
#else
	return __sync_val_compare_and_swap(dst, oldval, s);
#endif //_WIN32
}

inline long atomic_get(long volatile *dst)
{
#ifdef _WIN32
	return InterlockedExchange(dst, *dst);
#else
	return __sync_fetch_and_or(dst, 0);
#endif //_WIN32
}

