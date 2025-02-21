#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int main( int argc, char **argv){
    FILE *f, *f_out = stdout;
    char *filename, *outfname;
    char buf[1024], f_name[80];
    char *fp_name;
    char *p;
    int have_sep=0, have_text=0, n;

    if( argc < 2 || argc > 3){
        fprintf( stderr, "Usage: help_build.sh input_file output\n");
        fprintf( stderr, "Creates a C file out of text in input_file\n");
        return( 1);
    }

    filename = argv[1];
    if( ( f = fopen( filename, "r")) == NULL){
        perror("Could not open read file, abort\n");
        return( 1);
    }


    if( argc == 3){
        outfname = argv[2];
        if( ( f_out = fopen( outfname, "w+")) == NULL){
            perror("Could not create file, abort\n");
            return( 1);
        }
    }

    while( fgets( buf, 1023, f) != NULL){
        n = strlen( buf);
        if( n > 0){
            buf[n-1] = 0;
        }

        p = buf;
        have_sep = 0; 
        while( *p != 0){
            if( *p == '$' ){
                have_sep = 1;
                break;
            }
            p++;
        }


        if( have_sep){
            if( have_text){
                fprintf( f_out, "};\n\n");
            }

            have_text = 1;

            p = buf;
            fp_name = f_name;

            while( !isspace( *p++));
            while( !isspace( *p)){
                *fp_name++ = *p++;
            }
            *fp_name = 0;
            if( f_name == NULL){
                fprintf( stderr, "Should not happen, abort");
                return(1);
            }

            fprintf( f_out, "char *%s[]={\n", f_name);
            continue;
        }

        fprintf( f_out, "  \"%s\",\n", buf);
    }


    if( have_text){
        fprintf( f_out, "};\n\n");
    }

    fclose(f);
    if( f_out != stdin){
        fclose( f_out);
    }
    return(0);
}


