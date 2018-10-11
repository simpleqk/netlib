/**********************************************************
* file: log.h
* brief: simple log manager
* 
* author: qk
* email: 
* date: 2018-07-21
* modify date: 2018-08-23, add LOG_HAVE_FILE_INFO
**********************************************************/

#ifndef _NET_LOG_H_
#define _NET_LOG_H_

//log level type/mask
#define LOG_TYPE_ERROR 0x80  //10000000
#define LOG_TYPE_WARN  0x40  //01000000
#define LOG_TYPE_STAT  0x20  //00100000
#define LOG_TYPE_INFO  0x10  //00010000
#define LOG_TYPE_DEBUG 0x08  //00001000
#define LOG_TYPE_USR2  0x04  //00000100
#define LOG_TYPE_USR1  0x02  //00000010
#define LOG_TYPE_USR0  0x01  //00000001

//common level type
#define LOG_TYPE_COMMON (LOG_TYPE_ERROR|LOG_TYPE_WARN|LOG_TYPE_STAT|LOG_TYPE_INFO)

#ifdef LOG_HAVE_FILE_INFO
#define LOG_ERROR(fmt, arg...) log_write(LOG_TYPE_ERROR, "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#define LOG_WARN(fmt, arg...)  log_write(LOG_TYPE_WARN,  "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#define LOG_INFO(fmt, arg...)  log_write(LOG_TYPE_INFO,  "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#define LOG_STAT(fmt, arg...)  log_write(LOG_TYPE_STAT,  "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#define LOG_DEBUG(fmt, arg...) log_write(LOG_TYPE_DEBUG, "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#define LOG_USR2(fmt, arg...)  log_write(LOG_TYPE_USR2,  "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#define LOG_USR1(fmt, arg...)  log_write(LOG_TYPE_USR1,  "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#define LOG_USR0(fmt, arg...)  log_write(LOG_TYPE_USR0,  "%s:%d, "fmt, __FILE__, __LINE__, ##arg)
#else
#define LOG_ERROR(fmt, arg...) log_write(LOG_TYPE_ERROR, fmt, ##arg)
#define LOG_WARN(fmt, arg...)  log_write(LOG_TYPE_WARN,  fmt, ##arg)
#define LOG_INFO(fmt, arg...)  log_write(LOG_TYPE_INFO,  fmt, ##arg)
#define LOG_STAT(fmt, arg...)  log_write(LOG_TYPE_STAT,  fmt, ##arg)
#define LOG_DEBUG(fmt, arg...) log_write(LOG_TYPE_DEBUG, fmt, ##arg)
#define LOG_USR2(fmt, arg...)  log_write(LOG_TYPE_USR2,  fmt, ##arg)
#define LOG_USR1(fmt, arg...)  log_write(LOG_TYPE_USR1,  fmt, ##arg)
#define LOG_USR0(fmt, arg...)  log_write(LOG_TYPE_USR0,  fmt, ##arg)
#endif //LOG_HAVE_FILE_INFO

#define LOG_FLUSH()            log_flush()
#define LOG_CLOSE()            log_close()

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************
 * brief: init log envirement
 * input: name, log file name, if null or empty, default name is 'log'
 *        type_mask, set which type log will be writed in file
 * return: 0 ok, other error
 *********************************************************/
int log_init(const char *name, unsigned char type_mask);

/**********************************************************
 * brief: write data to log file
 * input: type, current log data type
 *        fmt, string format
 *        ..., param
 * return: 0 ok, other error
 *********************************************************/
int log_write(unsigned char type, char *fmt, ...);

/**********************************************************
 * brief: flush log buffer to file
 * input: none
 * return: none
 *********************************************************/
void log_flush();

/**********************************************************
 * brief: close log
 * input: none
 * return: none
 *********************************************************/
void log_close();

#ifdef __cplusplus
}
#endif

#endif //_NET_LOG_H_

