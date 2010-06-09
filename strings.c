#include <stdarg.h> /* varadic function */
#include <string.h> /* strlen, strcat etc */
#include <stdlib.h>


#include "strings.h"

#define STRING_ALLOC_SIZE 16

char *catstrings(int count, ...)
{
  char *ret;
  int i, len = 0;
  va_list list;

  va_start(list, count);

  for(i = 0; i < count; i++)
    len += strlen(va_arg(list, char *));

  ret = calloc(len + 1, sizeof(char));
  if(ret == NULL)
    return NULL;


  va_end(list);
  va_start(list, count);


  strcpy(ret, va_arg(list, char *));
  for(i = 1; i < count; i++)
    strcat(ret, va_arg(list, char *));

  return ret;
}


int addtostring(int c, int *nentered, int *cursize, char **s)
{
  if(IS_BACKSPACE(c)){
    if(*s && *nentered > 0){
      (*s)[--*nentered] = '\0';
      if(*nentered < 0){
        *nentered = 0;
      }
    }
    return 0;
  }
  /*
   * TODO: add ctrl+u/w processing for line and word erasing
   * TODO: add option for processing of special chars (don't want to do special chars in io.c
   */

  if(*nentered >= *cursize - 1){ /* '>' because 0 != 0-1 when the thread first gets here */
    char *tmp;

    *cursize += STRING_ALLOC_SIZE;

    tmp = realloc(*s, *cursize * sizeof(char));

    if(tmp == NULL)
      return 1;

    *s = tmp;
    memset(*s + *nentered, '\0', *cursize - *nentered);
  }

  (*s)[(*nentered)++] = c;

  return 0;
}
