#include "trace.h"


int main(){
    int n=31415; 
    TRACE_DBG("hola %d", n);
    TRACE_ERRNO( "iii=%d", 18);

    TRACE("esto no sirve = %d", 0);
    TRACE("hola");
}


