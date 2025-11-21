/*
.. $ gcc -o tr2 test.c tr2.c && ./tr2 tr2.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpp.h"

int main (int argc, char ** argv) {
  if (argc < 2) {
    fprintf (stderr, "fatal error : missing source");
    exit (-1);
  }
  char * source = argv [1];

  cpp_source (source);
  char * line;
  size_t nbytes = 0;
int max = 4;
  while ( (nbytes = cpp_fgets (&line)) && max--) {
    printf ("\n\n----------------\n%s\n------\n", line);
  }
  
}



