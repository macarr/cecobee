#ifndef ERRORS_H
#define ERRORS_H

int error_type_size = 50;
int error_description_size = 250;

typedef struct Errors {
  char http_status[3];
  char error_type[50];
  char error_description[250];
} Error;

void init();

Error* getError(char* error_desc);

#endif // ERRORS_H
