#include <string.h>
#include <stdint.h>
#include "trace.h"
#include "hash.h"
#include "dict.h"

char *dict_dt_names[]={ "none" , "int", "uint", "float", "boolean", "string", 
                        "blob", "extent", NULL };

char *dict_get_type_name( int dt){
    if( dt >= 0 || dt < DICT_NUM_TYPES){
        return( dict_dt_names[dt]);
    }

    return( NULL);
}

int dict_get_type_id( char *dt){
    int i;

    for( i = 0; i < DICT_NUM_TYPES; i++){
        if( strncmp( dict_dt_names[i], dt, 8) == 0){
            return( i);
        }
    }

    return( -1);
}


value_t dict_value_new( unsigned int data_type, void *data, int len){
    value_t v;
    char c;

    switch( data_type){
        case DICT_INT: 
            v.value.i = (*(int *)data);
            break;
        case DICT_UINT:
            v.value.ui =  (*(uint64_t *)data);
            break;
        case DICT_FLOAT: 
            v.value.f = (*(double *)data);
            break;
        case DICT_BOOLEAN:
            c = (*(char *)data);
            v.value.b = (c != 0);
            break;
        case DICT_STRING:
            v.value.s = strndup( ( char *) data, DICT_MAX_STRLEN);
            v.data_len = strnlen( v.value.s, DICT_MAX_STRLEN);
            break;
        case DICT_BLOB: 
        case DICT_EXTENT:
            v.value.s = ( char *) data;
            v.data_len = ( uint32_t) len;
            break;
        default:
            memset( &v, 0, sizeof( v));
            return( v);
    }
    v.data_type = data_type;

    return( v);
}


void dict_init( dict_t *d){
    d->n_elems = 0;
    d->used_elems = 0;
    d->dict = NULL;
}

dict_t dict_new(){
    dict_t d;

    dict_init( &d);
    return(d);
}


int dict_add_entry( dict_t *d, char *key, value_t value){
    dict_entry_t dentry; 
    void *p = NULL;
    int nbuflen = 0;

    strncpy( dentry.key, key, DICT_KEY_LEN);
    dentry.value = value;

    dentry.hash_key = hash_b79( dentry.key);

    /* if nothing in the dict, calculate a new buf length */ 
    if( d->dict == NULL){
        nbuflen = sizeof( dict_entry_t ) * DICT_DEFAULT_NUM_ELEMS;
    }else if( (d->used_elems + 1) >= d->n_elems){
        /* if we need extra storage, also calculate a new buf length */
        nbuflen = sizeof( dict_entry_t ) * d->n_elems * 2;
    }       

    if( nbuflen > 0){
        /* if extra memory is required, alloc and clean the new memory */
        d->dict = realloc( d->dict, nbuflen);
        if( p != NULL){
            TRACE_SYSERR("could not reserve memory");
            exit(1);
        }
        d->n_elems = nbuflen / sizeof( dict_entry_t);

        memset( &d->dict[d->used_elems], 
                0, 
                ( d->n_elems - d->used_elems) * sizeof( dict_entry_t )); 

    }

    memcpy( &d->dict[d->used_elems], &dentry, sizeof( dict_entry_t));
    d->used_elems++;

    return(0);
}

dict_entry_t *dict_search_entry( dict_t *d, char *key, int *sub){
    int i;

    for( i = 0; i < d->used_elems; i++){
        if( strncmp( key, d->dict[i].key, DICT_KEY_LEN) == 0){
            *sub = i;
            return( &d->dict[i]);
        }
    }

    return(NULL);
}





int dict_remove_entry( dict_t *d, char *key){
    dict_entry_t *o;
    int sub;

    if( d->used_elems == 0){
        return( -1);
    }

    o = dict_search_entry( d, key, &sub);
    if( o == NULL){
        return( -1);
    }

    if( (d->used_elems - 1) != sub){
        memcpy( &d->dict[sub], 
                &d->dict[sub+1], 
                sizeof( dict_entry_t) * ( d->used_elems - sub - 1));
    }
    d->used_elems--;
    return( 0);
}

int dict_update_entry( dict_t *d, char *key, value_t new_value){
    dict_entry_t *o;
    int sub;
    value_t v;

    if( d->used_elems == 0){
        return( -1);
    }

    o = dict_search_entry( d, key, &sub);
    if( o == NULL){
        return( -1);
    }
    v = o->value;
    if( v.value.s != NULL && 
        ( (v.data_type == DICT_BLOB   ) || 
          (v.data_type == DICT_EXTENT ) ||
          (v.data_type == DICT_STRING ) ) ){
        free( v.value.s);
    }
 
    o->value = new_value;
    return(0);
}

void dict_clean( dict_t *d){
    int i;
    value_t v;

    if( d->n_elems == 0 && d->dict == NULL){
        return;
    }

    /* free the dynamic memory of blobs, strings and extents */
    for( i = 0; i < d->used_elems; i++){
        v = d->dict[i].value;
        if( v.value.s != NULL && 
            ( (v.data_type == DICT_BLOB   ) || 
              (v.data_type == DICT_EXTENT ) ||
              (v.data_type == DICT_STRING ) ) ){
            free( v.value.s);
        }
    }

    free( d->dict);
    dict_init( d);
}


void dict_display( dict_t *d){
    int i;
    value_t v;

    printf("[");
    for( i = 0; i < d->used_elems; i++){
        if( i != 0){
            printf(",");
        }
        printf(" %s:", d->dict[i].key);
        v = d->dict[i].value;
        switch( v.data_type){
            case DICT_INT: 
                printf("%d ", v.value.i); 
                break;
            case DICT_UINT: 
                printf("%lu ", v.value.ui); 
                break;
            case DICT_FLOAT: 
                printf("%6.4f ", v.value.f); 
                break;
            case DICT_BOOLEAN: 
                printf("%c ", (0x30 + v.value.b)); 
                break;
            case DICT_STRING: 
                printf("'%s' ", v.value.s); 
                break;
            case DICT_BLOB: 
            case DICT_EXTENT:
                printf("BLOB/EXTENT "); 
                break;
            default: 
                printf(" WTF "); 
        }
    }
    printf("]\n");
}

