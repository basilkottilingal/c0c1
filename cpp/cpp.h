
#ifndef _H_CPP_
#define _H_CPP_ 1

  #ifndef CPP_PAGE_SIZE
    #define CPP_PAGE_SIZE (1<<14)
  #endif


  void   cpp_buff_size_set (size_t);
  size_t cpp_buff_size_get ();

  /*
  .. List of API
  .. -  const char * cpp_gets (size_t * len);
  ..     Return the buffer that holds one or more of the 
  ..     prepreprocessed line-  from the input source file.
  ..     "prepreprocessod" means tranlation phase 2 and 3.
  ..     Return NULL if EOF encountered.
  ..     *len is the length of the buffer excluding the trailing
  ..     NUL character
  .. -  void cpp_source (const char * file);
  ..     Set the input source file
  .. -  void cpp_clean ();
  ..     Clean all the buffer and close input file.
  */
  const
  char * cpp_gets   (size_t * len);
  void   cpp_source (const char * file);
  void   cpp_clean  ();

#endif
