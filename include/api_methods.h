#ifndef API_METHODS_H
#define API_METHODS_H
#include "../include/utils.h"

typedef struct header {
    char* header_name;
    char* header_value;
} header;

void init_header(header* h);

void setHeaderName(header* h, const char* name);

void setHeaderValue(header* h, const char* value);

typedef enum {GET, POST, DELETE, PUT, PATCH} http_method;

int callApi(char* endpoint, http_method method, char* queryParams, char* body, string* result, int numHeaders, header* headers);

#endif // API_METHODS_H
