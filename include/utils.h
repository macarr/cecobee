#ifndef UTILS_H
#define UTILS_H

typedef struct string {
  char* ptr;
  size_t len;
} string;

void init_string(string* s);

char* concat(const char* s1, const char* s2);

char* concat2(char* s1, const char* s2);

#endif // UTILS_H
