#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/utils.h"

void init_string(struct string* s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

char* concat(const char* s1, const char* s2)
{
    char* result;
    result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    if(!result) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char* concat2(char* s1, const char* s2) {
  s1 = realloc(s1, strlen(s1) + strlen(s2) + 1);//+1 for the zero-terminator
  if(!s1)  {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  strcat(s1, s2);
  return(s1);
}
