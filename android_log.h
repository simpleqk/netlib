/**********************************************************
* file: android_log.h
* brief: jni logger need link libaray log
*        LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
* 
* author: qk
* email: 
* date: 2018-09
* modify date: 
**********************************************************/

#ifndef _ANDROID_LOG_H_
#define _ANDROID_LOG_H_

#ifdef _ANDROID

#include <android/log.h>

/* 
 * Android log priority values, in ascending priority order. 
 * from system/core/include/android/log.h
 */
//typedef enum android_LogPriority {
//	ANDROID_LOG_UNKNOWN = 0,
//	ANDROID_LOG_DEFAULT, /* only for SetMinPriority() */
//	ANDROID_LOG_VERBOSE,
//	ANDROID_LOG_DEBUG,
//	ANDROID_LOG_INFO,
//	ANDROID_LOG_WARN,
//	ANDROID_LOG_ERROR,
//	ANDROID_LOG_FATAL,
//	ANDROID_LOG_SILENT, /* only for SetMinPriority(); must be last */
//}android_LogPriority;


#define LOG_TAG "pw_sdk"

#define LOG_d(fmt, arg...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG"_debug", fmt, ##arg)
#define LOG_i(fmt, arg...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG"_info",  fmt, ##arg)
#define LOG_w(fmt, arg...) __android_log_print(ANDROID_LOG_WARN,  LOG_TAG"_warn",  fmt, ##arg)
#define LOG_e(fmt, arg...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG"_error", fmt, ##arg)
#define LOG_s(fmt, arg...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG"_stat",  fmt, ##arg)
#define LOG_u2(fmt, arg...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG"_usr2",  fmt, ##arg)
#define LOG_u1(fmt, arg...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG"_usr1",  fmt, ##arg)
#define LOG_u0(fmt, arg...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG"_usr0",  fmt, ##arg)

#define LOGW_WRITE(prio, tag, buf) __android_log_write(prio, tag, buf)
#define LOGW_d(buf) LOGW_WRITE(ANDROID_LOG_DEBUG, LOG_TAG"_debug", buf)
#define LOGW_i(buf) LOGW_WRITE(ANDROID_LOG_INFO,  LOG_TAG"_info",  buf)
#define LOGW_w(buf) LOGW_WRITE(ANDROID_LOG_WARN,  LOG_TAG"_warn",  buf)
#define LOGW_e(buf) LOGW_WRITE(ANDROID_LOG_ERROR, LOG_TAG"_error", buf)
#define LOGW_s(buf) LOGW_WRITE(ANDROID_LOG_INFO,  LOG_TAG"_stat",  buf)
#define LOGW_u2(buf) LOGW_WRITE(ANDROID_LOG_INFO, LOG_TAG"_usr2",  buf)
#define LOGW_u1(buf) LOGW_WRITE(ANDROID_LOG_INFO, LOG_TAG"_usr1",  buf)
#define LOGW_u0(buf) LOGW_WRITE(ANDROID_LOG_INFO, LOG_TAG"_usr0",  buf)

#endif //_ANDROID

#endif //_ANDROID_LOG_H_

