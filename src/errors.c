#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/errors.h"

//internal datatype, very very very very shameful linked list
typedef struct ErrorNode {
  Error* err;
  struct ErrorNode* next;
} ErrorNode;

ErrorNode head;

/**
 * populate our error list with all the errors we can potentially recieve
 */
void init() {
  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  int group, pos, i;
  ssize_t read;
  int listSize = 0;
  Error* e;
  ErrorNode* node = &head;
  puts("Reading errors from file");

  fp = fopen("./errs", "r");
  if(fp == NULL)
    exit(EXIT_FAILURE);

  //each line is an error definition
  //the error is defined as <type>:<http status>:<description>
  while((read = getline(&line, &len, fp)) != -1) {
    //reset vars
    group = 0;
    pos = 0;
    e = malloc(sizeof(Error));
    for(i = 0; i < len; i++) {
      if(line[i] == ':') {
        group++;
        pos = 0;
      } else {
        switch(group) {
        case 0:
          e->error_type[pos] = line[i];
          break;
        case 1:
          e->http_status[pos] = line[i];
          break;
        case 2:
          e->error_description[pos] = line[i];
          break;
        default:
          puts("Malformed errors file: too many groups");
          exit(EXIT_FAILURE);
        }
        pos++;
      }
    }
    printf("Created error %s\n", e->error_type);
    //error created, add it to the list and ready it for the next iteration;
    node->err = e;
    node->next = malloc(sizeof(ErrorNode));
    node = node->next;
    listSize++;
  }
  node->err = NULL;
  node->next = NULL;
  printf("Successfully read in %i errors\n", listSize);
  //list is null-terminated
  fclose(fp);
  if(line)
    free(line);
}

/**
 * Given an error description, get the full data on the relevant error
 */
Error* getError(char* error_desc) {
  ErrorNode* pos = &head;
  while(pos->err != NULL) {
    if(strcmp(pos->err->error_type, error_desc) == 0) {
      return pos->err;
    }
    pos = pos->next;
  }
  return NULL;
}

#ifdef DEBUG
void main(void) {
  init();
  puts("TEST============");
  Error* err = getError("account_disabled");
  if(err != NULL) {
    printf("Desc:%s\nType:%s\nStatus:%s\n", err->error_description, err->error_type, err->http_status);
  } else {
    puts("failed. could not find account_disabled error");
  }
  exit(0);
}
#endif
