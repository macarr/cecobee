#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/api_methods.h"
#include "../libs/curl/curl.h"

#define GET 1
#define POST 2
#define DELETE 3
#define PUT 4
#define PATCH 5



size_t writefunc(void *ptr, size_t size, size_t nmemb, string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

int callApi(char* endpoint, http_method method, char* queryParams, char* body, string* s) {
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if(curl) {
    //init_string(s);
    char* url = "https://api.ecobee.com/";
    url = concat(url, endpoint);
    url = concat2(url, "?");
    url = concat2(url, queryParams);
    printf("Url and Queries: %s\nBody:%s\n", url, body);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, s);
    switch(method) {
    case POST:
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        break;
    case DELETE:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    case PUT:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        break;
    case PATCH:
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        break;
    default:
        break;
        /* Does a GET by default, don't have to do anything here and we'll assume invalid methods are GETs */
    }
    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    long http_code = 0;
    /* Check for errors */
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      return 2;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("\nHTTP status: %lu\n", http_code);

    /* always cleanup */
    curl_easy_cleanup(curl);
    if(http_code < 200 || http_code > 299) {
      return 3;
    } else {
      return 0;
    }
  } else {
    return 1;
  }
}
