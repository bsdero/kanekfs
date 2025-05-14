#ifndef _TRACE_H_
#define _TRACE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#define TRC_LVL_ALL              0
#define TRC_LVL_DEBUG            1
#define TRC_LVL_INFO             2
#define TRC_LVL_NOTICE           3
#define TRC_LVL_WARNING          4
#define TRC_LVL_ERROR            5
#define TRC_LVL_CRITICAL         6
#define TRC_LVL_ALERT            7
#define TRC_LVL_EMERGENCY        8


typedef struct{
    uint64_t class;
    uint16_t level;
}trace_t;



#define TRACE_DBG(fmt,...)       fprintf( stdout, "%s:%s:%d: "fmt"\n",      \
                                          __FILE__, __FUNCTION__, __LINE__, \
                                          ##__VA_ARGS__);                   \
                                 fflush( stdout)  

#define TRACE_ERR(fmt,...)       fprintf( stderr, "ERROR:%s:%s:%d: "fmt"\n",\
                                          __FILE__, __FUNCTION__, __LINE__, \
                                          ##__VA_ARGS__);                   \
                                 fflush( stderr)  

#define TRACE_SYSERR(fmt,...)    fprintf( stderr,                           \
                                          "SYSERR:%d:%s:%s:%s:%d: "fmt"\n", \
                                          errno, strerror( errno),          \
                                          __FILE__, __FUNCTION__, __LINE__, \
                                          ##__VA_ARGS__);                   \
                                 fflush( stdout)  

#define TRACE_ERRNO(fmt,...)     fprintf( stderr,                           \
                                          "ERRNO: %d:%s: "fmt"\n",          \
                                          errno, strerror( errno),          \
                                          ##__VA_ARGS__);                   \
                                 fflush( stderr)

#define TRACE_STR( str, fmt,...) sprintf( str, "%s:%s:%d: "fmt"\n",         \
                                          __FILE__, __FUNCTION__, __LINE__, \
                                          ##__VA_ARGS__); 
                                          

#define TRACE( file, class, level, str, fmt, ...)   {                        \
                                 if((level >= global_trace.level) &&         \
                                    ( (class & global_trace.class) != 0)){   \
                                        TRACE_STR( str, fmt, ##__VA_ARGS__); \
			                            trace(file, fmt, ##__VA_ARGS__);     \
                                    } \

int trace( FILE *file, char *string);
#endif
