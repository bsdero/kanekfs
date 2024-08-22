#include <string.h>
#include "dict.h"


int main(){
    dict_t d;
    int i;
    uint64_t ui; 
    double f;
    char c;
    char s[45];

    d = dict_new();
    dict_display( &d);

    i = -1;
    dict_add_entry( &d, "uno", dict_value_new( DICT_INT, &i, 0));
    dict_display( &d);

    i = 2;
    dict_add_entry( &d, "dos", dict_value_new( DICT_INT, &i, 0));

    f = 33.3333333;
    dict_add_entry( &d, "tres", dict_value_new( DICT_FLOAT, &f, 0));

    ui = 444444;
    dict_add_entry( &d, "four", dict_value_new( DICT_UINT, &ui, 0));

    c = 0;
    dict_add_entry( &d, "cinque", dict_value_new( DICT_BOOLEAN, &c, 0));

    c = 1;
    dict_add_entry( &d, "six", dict_value_new( DICT_BOOLEAN, &c, 0));

    c = 61;
    dict_add_entry( &d, "siete", dict_value_new( DICT_BOOLEAN, &c, 0));

    strcpy( s, "eight");
    dict_add_entry( &d, "octo", dict_value_new( DICT_STRING, s, 0));

    strcpy( s, "9a9b9c9d9e9f9a9c9b9d9e9f999999999999999999");
    dict_add_entry( &d, "novem", 
            dict_value_new( DICT_STRING, s, 0));


    dict_display( &d);

    dict_remove_entry( &d, "six");
    dict_display( &d);

    strcpy( s, "ochoocho");
    dict_update_entry( &d, "octo", dict_value_new( DICT_STRING, s, 0));
    dict_display( &d);

    dict_clean( &d);
    dict_display( &d);
    return 0;
}

