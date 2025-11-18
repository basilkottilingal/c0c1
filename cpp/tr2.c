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

#include "cpp.h"

/*
.. fread and put in buffer, if buffer is full, create a new buffer
.. and stack up buffers. Each "tkn" should be contiguous in memory.
.. NOTE : Each "tkn" in the input buffer corresponds to a line after
.. removing splicing ( "\\\n" ). The corresponding prepreprocessed
.. output should also be contiguous.
*/
typedef struct BuffStack {
  char * buff;
  struct BuffStack * next;
} BuffStack;
static BuffStack * head = NULL;
static char tkn_dummy [] = "";
static FILE * fp  = NULL;
static char * tkn = tkn_dummy;
static char * eob = tkn_dummy;
static char * ptr = tkn_dummy;
static char * eof = NULL;

#define oom(_c_)                                         \
  do {                                                   \
    if (_c_) {                                           \
      fprintf(stderr, "cpp : out of dynamic memory");    \
      exit (-1);                                         \
    }                                                    \
  } while (0)

static void input_stack () {

  if (!fp) {
    fprintf (stderr, "\ncpp : no input source");
    exit (-1);
  }

  if (eof == eob)
    return;

  char * buff = head ? head->buff : NULL;
  size_t non_parsed = eob - tkn,
    size = page_size;

  if (buff == tkn) {
    size = 2 * non_parsed;
    head->buff = buff = realloc (buff, size);
    oom (!buff);
  }
  else {
    BuffStack * s = malloc (sizeof (BuffStack));
    buff = malloc (size);
    oom (!s || !buff);
    s->buff = buff;
    s->next = head;
    head = s;
    memcpy (buff, tkn, non_parsed);
  }

  size_t n = non_parsed + fread (ptr, 1, size - non_parsed, fp);
  tkn = buff;
  ptr = buff + non_parsed;
  eob = buff + n;
  if (n < size) {
    if (!feof (fp)) {
      fprintf (stderr, "cpp : fread () failed");
      exit (-1);
    }
    /*
    .. fixme :  truncate the buff, if very conservative use of memory.
    */
    eof = eob;
  }
}

static int input () {
  if (ptr == eob) {
    input_stack ();
    if (eob == eof)
      return EOF;
  }
  return (int) (unsigned char) *ptr++;
}

/*
.. prepreprocessed output is the output of translation phases 2-3 
.. (Refer steps (a)-(f)) which is temporarily stored in "buff" and
.. returned when cpp_fgets () is called.
.. NOTE : "buff" may hold 1 or more lines that can fit in |buff|.
*/
static char * buff;
static char * _ptr;
static char * _tkn;
static char * _eob;
static char holdchar = '\0';

static inline
int push (int c) {
  *_ptr++ = (char) (unsigned char) c;

  if (_ptr == _eob) {
    if (_tkn == buff) {
      size_t s = (size_t) 1 + (_eob - _tkn);
      _tkn = buff = realloc (buff, 2*s);
      oom (!buff);
      _ptr = buff + (s-1);
      _eob = buff + (2*s-1);
    }
    else {
      holdchar = *_tkn;
      *_tkn = '\0';
      return 0;
    }
  }

  return 1;
}

static int eol (int quote) {
      
  char * p = _ptr, c;
  if (quote) {
    while ( (c = *--p) != '"') {
      if ( !(c == ' ' || c == '\t') )
        break;
    }
    if (c == '\\') {
      //fixme : stack warning. white space after "\\"
      _ptr = p;
      return 0;
    }
    //fixme : stack warning. unclosed quote
    return 1;
  }

  /*
  .. pop trailing white spaces, and splicing "\\\n" if found
  */
  int splicing = 0;
  while (p-- != _tkn) {
    if ((c=*p) == '\\') {
      if (splicing)
        break;
      //if (_ptr-p>1) error ("trailing whitespace before '\\'");
      splicing = 1;
      continue;
    }
    if (!(c == ' '||c == '\t'))
      break;
  }
  _ptr = p+1;
  return !splicing;
}

void cpp_source (const char * source) {
  if (!source) {
    fp = stdin;
    return;
  }
  fp = fopen (source, "r");
  if (!fp) {
    fprintf (stderr, "cpp : fatal error : cannot open source");
    exit(-1);
  }
  _tkn = _ptr = buff = malloc (page_size);
  oom (!buff);
  _eob = & _ptr [page_size-1];
}

static int quote   = 0;
static int escape  = 0;
static int comment = 0;
static int percent = 0;
static int compensate = 0;

int cpp_gets () {

  int c;
  while ( (c=input()) != EOF ) {
    push (c);

    if (c == '\n') {
      compensate ++;
      if (eol (quote)) {
        for (;compensate;--compensate)
          push ('\n');
        quote = 0;
        /* New line. So new token */
        _tkn = _ptr;
        tkn = ptr;
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
        while ( (_ptr != _tkn) ) {
          if ( ! ((c = _ptr[-1]) == '\\' || c == '\t' ) )
            break;
          pop();
        }
        for (++compensate; compensate; --compensate)
          push ('\n');
        _tkn = _ptr;
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
        char * p = _ptr - 2;
        /* check if ^[ \t]*"%:" */
        while (p != _tkn) {
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
