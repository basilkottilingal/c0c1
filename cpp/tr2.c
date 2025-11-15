/*
.. Translation phases 2 and 3.
.. (a) Replaces multiline comment "\*".*"*\" with " "
.. (b) Consumes single line comment "\\".*,
.. (c) Replaces "%:" with "#"
.. (d) Replaces "%:%:" with "##"
.. (e) Consumes line splicing [\\][\n][ \t]*$
.. (f) Consumes trailing white spaces [ \t]+$
.. Assumes translation phase 1 is obsolete (and not required).
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
.. fread and put in buffer, if buffer is full, create a new buffer
.. and stack up buffers.
*/
static BuffStack * head = NULL;
typedef struct BuffStack {
  char * buff;
  struct BuffStack * next;
} BuffStack;
static FILE * fp;
static char yytext_dummy [] = "";
static char * yytext = yytext_dummy;
static char * eob    = yytext_dummy;
static char * ptr    = yytext_dummy;
static char * eof    = NULL;

/*
.. prepreprocessed output
*/
static char * buff;
static char * bptr;
static char * strt;
static char * lim;


#define ACCEPT() do {   \
    *yytext = holdchar; \
    yytext = ptr;       \
    holdchar = *yytext; \
    *yytext = '\0';     \
  } while (0)

#define EOB() do {                                       \
    if ( strt == buff )                                  \
      if ( (buff = realloc (buff, lim-buff)) == NULL) {  \
        fprintf (stderr, "\ncpp : realloc () failed");   \
        exit (-1);                                       \
      }                                                  \
  }

#define YYSTART() do {  \
    if (ptr == yytext)  \
      break;            \
     
  } while (0)

static int input_stack () {

  if (!fp) {
    fprintf(stderr, "cpp : internal error : missing input file");
    exit (-1);
  }

  BuffStack * s = head;
  char * buff = head ? head->buff : NULL;
  size_t size = eob-yystart;

  if ( buff == yytext) {
    buff = realloc ( buff, 2*size );
    if ( !buff ) {
      fprintf(stderr, "cpp : out of dynamic memory");
      exit (-1);
    }
    yytext = head->buff = buff;
    eob = buff + 2*size ;
    ptr = buff + size;
  }
  else {
    s = malloc (sizeof (BuffStack));
    buff = malloc (page_size);
    if (!s || !buff) {
      fprintf(stderr, "cpp : out of dynamic memory");
      exit (-1);
    }
    s->buff = buff;
    eob = buff + page_size;
    yystart = buff 
  }

  head = s;
  size_t n = fread (s->buff, 1, page_size - 1, fp);
  eob = s->buff + n;
  ptr = s->buff;
  if (n < page_size - 1) {
    if (!feof (fp)) {
      fprintf (stderr, "fread () failed");
      exit (-1);
    }
    eof = s->buff + n;
    if (!n) return 1;
  }
  return 0;
}

static int input () {
  if (ptr == eob) {
    if (ptr == eof || input_stack ())
      return EOF;
  }
  return (int) (unsigned char) *ptr++;
}

static void echo () {
  fwrite (buff, 1, strt-buff, stdout);
  memmove (buff, strt, bptr-strt);
  bptr = & buff [bptr-strt];
  strt = buff;
}

static void push (int c) {
  /*
  .. The current line is guaranteed in the buffer.
  .. "strt" is the BOL (current line)
  */
  if (bptr == lim) {
    size_t size = (lim - buff) + 1;
    if ( bptr-strt > (size_t) (0.8* size) ) {
      strt = buff = realloc (buff, 2*size);
      if (!buff) {
        fprintf(stderr, 
          "cpp : out of dynamic memory : malloc failed");
        exit (-1);
      }
      lim = & buff [2*size-1];
      bptr = & buff [size-1];
    }
    else
      echo ();
  }
  *bptr++ = (char) (unsigned char) c;
}

#define pop()      bptr--
static int eol (int quote) {
  pop();
      
  char * p = bptr, c;
  if (quote) {
    while ( (c = *--p) != '"') {
      if ( !(c == ' ' || c == '\t') )
        break;
    }
    if (c == '\\') {
      //stack warning. white space after "\\"
      bptr = p;
      return 0;
    }
    //stack warning. unclosed quote
    return 1;
  }

  /*
  .. pop trailing white spaces, and splicing "\\\n" if found
  */
  int splicing = 0;
  while (p-- != strt) {
    if ((c=*p) == '\\') {
      if (splicing)
        break;
      //if (bptr-p>1) error ("trailing whitespace before '\\'");
      splicing = 1;
      continue;
    }
    if (!(c == ' '||c == '\t'))
      break;
  }
  bptr = p+1;
  return !splicing;
}


int main (int argc, char ** argv) {
  if (argc < 2) {
    fprintf (stderr, "fatal error : missing source");
    exit (-1);
  }
  char * source = argv [1];
  fp = fopen (source, "r");
  if (!fp) {
    fprintf (stderr, "fatal error : cannot open source");
    exit(-1);
  }

  strt = bptr = buff = malloc (page_size);
  if (!buff) {
    fprintf(stderr, "out of dynamic memory : malloc failed");
    exit (-1);
  }
  lim = & bptr [page_size-1];
  int c, quote = 0, escape = 0, comment = 0,
    percent = 0, compensate = 0;
  char * backup;
  while ( (c=input()) != EOF ) {
    push (c);

    if (c == '\n') {
      compensate ++;
      if (eol (quote)) {
        for (;compensate;--compensate)
          push ('\n');
        strt = bptr;
        quote = 0;
      }
      escape = 0;
      continue;
    }

    if (quote) {
      if (escape)
        escape = 0;
      else if (c == '\\')
        escape = 1;
      else if (c == quote)
        quote = 0;
      continue;
    }

    if (comment) {
      comment  = 0;
      if ( c == '/' ) {
        pop (); pop ();
        /* consuming single line comment */
        while ( (c = input()) != EOF ) {
          if ( c == '\n' )
            break;
        }
        /* consume white space before "\\" */
        while ( (bptr != strt) ) {
          if ( ! ((c = bptr[-1]) == '\\' || c == '\t' ) )
            break;
          pop();
        }
        for (++compensate; compensate; --compensate)
          push ('\n');
        strt = bptr;
        continue;
      }
      else if ( c == '*') {
        pop (); pop ();
        /* consuming multi line comment */
        while ( (c = input()) != EOF ) {
          if ( c == '\n') compensate++;
          if ( c != '*' ) continue;
          while ( (c = input()) == '*' ) {
          }
          if ( c == '/' ) break;
          if ( c == '\n') compensate++;
        }
        push (' ');
        continue;
      }
    }

    /* converting digraph "%:" to "#" */
    if (percent) {
      percent = 0;
      if (c == ':') {
        char * p = bptr - 2;
        /* check if ^[ \t]*"%:" */
        while (p != strt) {
          if (!(*--p == ' '|| *p == '\t')) {
            p = NULL; break;
          }
        }

        //if (!p) {
        //}

        if (p) {
          pop (); pop ();
          push ('#');
        }
        continue;
      }
    }

    switch (c) {
      case '/' :
        comment = 1;
        break;
      case '"'  :
      case '\'' :
        quote = c;
        break;
      case '%' :
        percent = 1;
        break;
      default :
        break;
    }
  }

  /* flush the left over characters in buff */
  echo ();

  /* cleaning */
  fclose(fp);
  BuffStack * s = head;
  while (s){
    BuffStack * next = s->next;
    free (s->buff);
    free (s);
    s = next;
  }
  free (buff);
}
