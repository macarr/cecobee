#ifndef API_METHODS_H
#define API_METHODS_H
#include "../include/utils.h"

typedef enum {GET, POST, DELETE, PUT, PATCH} http_method;

int callApi(char* endpoint, http_method method, char* queryParams, char* body, string* s);

#endif // API_METHODS_H
