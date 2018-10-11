/**********************************************************
* file: sleep.h
* brief: precise timing for sleep use select model
* 
* author: qk
* email: 
* date: 2018-09
* modify date: 
**********************************************************/

#ifndef _SLEEP_H_
#define _SLEEP_H_

//return 0-ok, -1 error
int sleeps(unsigned long seconds);
int sleepm(unsigned long milliseconds);
int sleepu(unsigned long microseconds);

#endif //_SLEEP_H_

