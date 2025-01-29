#include <ctype.h>
#include <string.h>



/* Cut spaces */
char *trim (char *s){
  /* Initialize start, end pointers */
  char *s1 = s;
  char *s2;

  s2 = s + strlen(s) - 1;

  /* Trim and delimit right side */
  while ( (isspace( (int) *s2)) && (s2 >= s1) ){
      s2--;
  }
  *(s2+1) = '\0';

  /* Trim left side */
  while ( (isspace( (int) *s1)) && (s1 < s2) ){
      s1++;
  }

  /* Copy finished string */
  strcpy (s, s1);
  return( s);
}



