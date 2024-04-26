#ifndef _TRACE_H_
#define _TRACE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


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


#define TRACE                    TRACE_DBG

#endif
