#include "log.h"
#include <stdio.h>
#include <string.h>
//#include <io.h>   //windows,_access
#include <unistd.h> //linux,access
#include <sys/stat.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

#ifdef _ANDROID
#include "android_log.h"
#endif //_ANDROID

#define LOG_FILE_MAX_COUNT (2)
#define LOG_FILE_MAX_SIZE (2*1024*1024)
#define LOG_FILE_NAME_LEN (256)

//default log type mask
static unsigned char g_log_type_mask = LOG_TYPE_COMMON;

//name tail reserve 2 character for file suffix: .1, .2, ...
static char g_log_filename[LOG_FILE_NAME_LEN+2] = "log";
//filename string len for add suffix quickly
static short g_log_filename_len = 3; //"log"
//current log suffix name order
static short g_log_suffix_num=0;
//current log size
static int g_log_file_size;
//log file handle
static FILE *g_log_fd;
//multi-thread synchro lock
static pthread_mutex_t g_log_mutext=PTHREAD_MUTEX_INITIALIZER;

static inline
int log_file_size(const char *filename)
{
	struct stat sa;
	return (-1==stat(filename, &sa)) ? 0 : sa.st_size;
}

static unsigned int log_choose_filename_suffix()
{
	register unsigned int n = LOG_FILE_MAX_COUNT;
	register short name_len = g_log_filename_len;

	g_log_filename[name_len] = '.';
	for(;n>0;--n) {
		g_log_filename[name_len+1] = n+48; //'0'<->48
		if(0==access(g_log_filename, F_OK)) {
			//file exist
			break;
		}
	}
	g_log_filename[name_len] = '\0';

	return n;
}

static inline
FILE* log_open_file(int next)
{
	short name_len = g_log_filename_len;

	//set current file name
	g_log_filename[name_len] = '.';
	g_log_filename[name_len+1] = g_log_suffix_num+48;//'0'<->48

	//get current file size
	if(next) {
		g_log_file_size = log_file_size(g_log_filename);
		return fopen(g_log_filename, "w+");
	}
	else {
		g_log_file_size = 0;
		return fopen(g_log_filename, "w");
	}
}

static inline 
FILE* log_open_next_file()
{
	g_log_suffix_num = (g_log_suffix_num+1) % LOG_FILE_MAX_COUNT;
	return log_open_file(1);
}

static
int log_format_msg(unsigned char type, char *fmt, va_list vl, char *buf, unsigned int len)
{
	int count1, count2;
	time_t systime;
	struct tm now_tm;
	const char* str_type="";

	systime = time(0);
	localtime_r(&systime, &now_tm);

	switch(type) {
	case LOG_TYPE_ERROR:
		str_type = "ERROR";
		break;
	case LOG_TYPE_WARN:
		str_type = "WARN ";
		break;
	case LOG_TYPE_STAT:
		str_type = "STAT ";
		break;
	case LOG_TYPE_INFO:
		str_type = "INFO ";
		break;
	case LOG_TYPE_DEBUG:
		str_type = "DEBUG";
		break;
	case LOG_TYPE_USR2:
		str_type = "USR2 ";
		break;
	case LOG_TYPE_USR1:
		str_type = "USR1 ";
		break;
	case LOG_TYPE_USR0:
		str_type = "USR0 ";
		break;
	}

	count1 = snprintf(buf, len, "[%04d-%02d-%02d %02d:%02d:%02d]    %s    ",
		now_tm.tm_year+1900, now_tm.tm_mon+1, now_tm.tm_mday, 
		now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec,
		str_type);
	if(count1>0) {
		count2 = vsnprintf(buf+count1, len-count1-1, fmt, vl);
		if(count2>0) {
			buf[count1+count2] = '\n';
			count2++;
		}
		else {
			return 0;
		}
	}
	else {
		return 0;
	}

	return count1+count2;
}


int log_init(const char *name, unsigned char type_mask)
{
	register char *dst = g_log_filename;
	int len=0;

	if(name && '\0'!=name[0]) {
		//make sure log_filename string tail is '\0'
		while(1) {
			if( (len<LOG_FILE_NAME_LEN-1)
			  && ('\0' != (*dst++ = *name++)) ) {
				len++;
				continue;
			}
			else {
				*dst = '\0';
				break;
			}
		}
		g_log_filename_len = len;
	}

	//ERROR mask must
	g_log_type_mask = LOG_TYPE_ERROR|type_mask;

	//choose current log name suffix order
	g_log_suffix_num = log_choose_filename_suffix();

	return 0;
}

int log_write(unsigned char type, char *fmt, ...)
{
	int ret = 0;
	static char buf[1024*8];
	va_list vl;
	register int len, write_len;

	if(type & g_log_type_mask) {
		if(NULL==fmt) {
			return -1;
		}
#ifdef _ANDROID
		//format log
		va_start(vl, fmt);
		////len = log_format_msg(type, fmt, vl, buf, sizeof(buf));
		len = vsnprintf(buf, sizeof(buf), fmt, vl);
		va_end(vl);
		(void)write_len;
		(void)log_open_next_file;
		(void)log_format_msg;
		switch(type) {
			case LOG_TYPE_USR0:  LOGW_u0(buf); break;
			case LOG_TYPE_USR1:  LOGW_u1(buf); break;
			case LOG_TYPE_USR2:  LOGW_u2(buf); break;
			case LOG_TYPE_DEBUG: LOGW_d(buf); break;
			case LOG_TYPE_INFO:  LOGW_i(buf); break;
			case LOG_TYPE_STAT:  LOGW_s(buf); break;
			case LOG_TYPE_WARN:  LOGW_w(buf); break;
			case LOG_TYPE_ERROR: LOGW_e(buf); break;
		}
#else 
		//multi-thread protect
		pthread_mutex_lock(&g_log_mutext);
		do {
			if(NULL==g_log_fd) {
				//open log file, maybe exist
				g_log_fd = log_open_file(0);
			}

			if(g_log_fd) {
				//write log
				//format log
				va_start(vl, fmt);
				len = log_format_msg(type, fmt, vl, buf, sizeof(buf));
				va_end(vl);

				//write to file
				////__android_log_write(prio, tag, buf);
				write_len = fwrite(buf, 1, len, g_log_fd);
				if(write_len < len) {
					//maybe disk is full, close log
					fclose(g_log_fd);
					g_log_fd = NULL;
					ret = -1;
					break;
				}

				//file size is up to limit size
				g_log_file_size += write_len;
				if(g_log_file_size>=LOG_FILE_MAX_SIZE) {
					fclose(g_log_fd);
					g_log_fd = log_open_next_file();
				}
				ret = len;
			}
			else {
				ret = -1;
				break;
			}
		}while(0);
		pthread_mutex_unlock(&g_log_mutext);
#endif //_ANDROID
	}

	return ret;
}

void log_flush()
{
#ifdef _ANDROID
#else
	pthread_mutex_lock(&g_log_mutext);
	if(g_log_fd) {
		fflush(g_log_fd);
	}
	pthread_mutex_unlock(&g_log_mutext);
#endif //_ANDROID
}

void log_close()
{
#ifdef _ANDROID
	(void)g_log_mutext;
	(void)g_log_fd;
#else
	pthread_mutex_lock(&g_log_mutext);
	if(g_log_fd) {
		fclose(g_log_fd);
		g_log_fd = NULL;
	}
	pthread_mutex_unlock(&g_log_mutext);
#endif //_ANDROID
}

