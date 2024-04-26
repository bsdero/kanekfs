#ifndef _GC_H_
#define _GC_H_

#include <stdlib.h>
#include <stddef.h>
#include "list.h"


/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

typedef struct{
    list_t list;
    uint32_t size;
    uint32_t mark;
#define MAX_DBG_STR_LEN                            48
    char d_str[MAX_DBG_STR_LEN];
    char data[];
}gc_node_t;



typedef gc_node_t gc_list_t;

int     gc_list_init( gc_list_t *ll);
void    gc_list_destroy( gc_list_t *ll);
size_t  gc_list_total_mem( gc_list_t *ll);
void   *gc_malloc( gc_list_t *ll, size_t size);
void    gc_free( void *p);
void   *gc_realloc( gc_list_t *ll, void *ptr, size_t size);
char   *gc_strdup( gc_list_t *ll, char *p);
char   *gc_strncat( gc_list_t *ll, char *p, char *q);
int     gc_dump_list( gc_list_t *ll);
void   *gc_calloc( gc_list_t *ll, size_t nelements, size_t elementSize);
void    gc_mark( void *ptr);
void    gc_sweep(gc_list_t *ll);
char   *gc_strndup( gc_list_t *ll, char *p, int n);
void   *gc_memclone( gc_list_t *ll, void *p, int n);
void   gc_node_set_trace( void *n, char *str);


#endif
