#ifndef _DICT_H_
#define _DICT_H_

#include <stdint.h>


#define DICT_MIN_STRING_LEN                        8
#define DICT_KEY_LEN                               32
#define DICT_DEFAULT_NUM_ELEMS                     4

typedef union value_u{
    int i;
    double f;
    char *s; /* null terminated string */
    char b;
    uint64_t ui;
}value_u;


typedef struct{
    unsigned char data_type;
#define DICT_NULL                                  0
#define DICT_INT                                   1
#define DICT_UINT                                  2
#define DICT_FLOAT                                 3
#define DICT_BOOLEAN                               4 /* in value.b */
#define DICT_STRING                                5
#define DICT_BLOB                                  6
#define DICT_EXTENT                                7

#define DICT_NUM_TYPES                             8

#define DICT_MAX_STRLEN                            250

    /* For the data types DICT_BLOB and DICT_EXTENT data should be stored in 
     * the field value.s.  
     */

    uint32_t data_len;
    value_u value;
}value_t;



typedef struct{
    uint64_t hash_key;
    char key[DICT_KEY_LEN];
    value_t value;
}dict_entry_t;


typedef struct{
    uint32_t n_elems;
    uint32_t used_elems;
    dict_entry_t *dict;
}dict_t; 


value_t dict_value_new( unsigned int data_type, void *data, int len);
dict_t dict_new();
void dict_init( dict_t *d);
int dict_add_entry( dict_t *d, char *key, value_t value);
int dict_remove_entry( dict_t *d, char *key);
dict_entry_t *dict_search_entry( dict_t *d, char *key, int *sub);
int dict_update_entry( dict_t *d, char *key, value_t new_value);
void dict_clean( dict_t *d);
void dict_display( dict_t *d);

int dict_get_type_id( char *dt);
char *dict_get_type_name( int dt);

#endif

