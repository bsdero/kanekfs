#ifndef _DUMPHEX_H_
#define _DUMPHEX_H_
#ifdef __GNUC__
#include <sys/types.h>
#endif
#ifdef __MINGW32__
#include <inttypes.h>
#endif
#include <stdint.h>

int dumphex( void *ptr, size_t size);
void dump_uint32( void *ptr, size_t size);
void dump_uint64( void *ptr, size_t size);


#endif
