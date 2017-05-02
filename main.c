#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libs/curl/curl.h"
#include "libs/json-parser/json.h"

#define GET 1
#define POST 2
#define DELETE 3
#define PUT 4
#define PATCH 5

struct string {
  char *ptr;
  size_t len;
};

struct authorizations {
  char *access;
  char *refresh;
};

void init_auth(struct authorizations *auth) {
    auth->access = malloc(1);
    auth->refresh = malloc(1);
    auth->access[0] = '\0';
    auth->refresh[0] = '\0';
}

char* concat(const char *s1, const char *s2)
{
    char* result;
    result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    if(!result) {
        exit(1);
    }
    strcpy(result, s1);
    strcat(result, s2);
    printf("CONCAT1: %s\n", result);
    return result;
}

void concat2(char *s1, const char *s2) {
    char temp[strlen(s1)];
    strcpy(temp, s1);
    printf("%s:%s\n", temp, s2);
    s1 = realloc(s1, strlen(temp) + strlen(s2) + 1);//+1 for the null terminator
    if(!s1) {
        exit(1);
    }
    strcpy(s1, temp);
    strcat(s1, s2);
    printf("CONCAT2: %s\n", s1);
}

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
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

void callApi(char* endpoint, int method, char* queryParams, char* body, struct string* s) {
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if(curl) {
    //init_string(s);
    char* url = "https://api.ecobee.com/";
    url = concat(url, endpoint);
    printf("wait %s wtffff", url);
    concat2(url, "?");
    concat2(url, queryParams);
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
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    printf("\nHTTP status: %lu\n", http_code);

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  printf("Hello?%s\n", s->ptr);
}

/**
 * checks to see if the provided json_object contains the provided key
 **/
int json_contains_key(json_value* json, char* key) {
    if(json->type != json_object) {
        return -1;
    }
    int numKeys = json->u.object.length;
    int i;
    for(i = 0; i < numKeys; i++) {
        if(strcmp(key, json->u.object.values[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * step one of authorizing the app
 **/
char* getPin(char* apiKey) {

    char* queryParam= "response_type=ecobeePin&client_id=";
    queryParam = concat(queryParam, apiKey);
    concat2(queryParam, "&scope=smartWrite");
    struct string* s;
    init_string(s);
    callApi("authorize", GET, queryParam, "", s);
    if(s->ptr != NULL) {
        json_char* json = (json_char*)s->ptr;
        json_value* value = json_parse(json, s->len);
        if (value == NULL) {
            fprintf(stderr, "Unable to parse data\n");
            exit(1);
        }
        int pinPos = json_contains_key(value, "ecobeePin");
        if(pinPos >= 0 && value->u.object.values[pinPos].value->type == json_string) {
            printf("Pin:%s\nPlease activate this under the 'My Apps' section of your ecobee dashboard. Press Any Key after you have done so.\n", value->u.object.values[pinPos].value->u.string.ptr);
            getchar();
        }
        int codePos = json_contains_key(value, "code");
        if(codePos >= 0 && value->u.object.values[codePos].value->type == json_string) {
            free(queryParam);
            return value->u.object.values[codePos].value->u.string.ptr;

        }
    }
    free(s);
    free(queryParam);
    return "";
}

/**
 * step two of authorizing the app. Refresh token is for regenerating
 * authorization tokens, authorization tokens are for making the actual
 * api requests
 **/
struct authorizations getTokens(char* apiKey, char* authCode) {
    char* body = "grant_type=ecobeePin&code=";
    body = concat(body, authCode);
    printf("%s!!!!!!!!!!\n", body);
    concat2(body, "&client_id=");
    printf("%s!!!!!!!!!!\n", body);
    concat2(body, apiKey);
    printf("%s!!!!!!!!!!\n", body);
    struct string* s;
    init_string(s);
    callApi("token", POST, "", body, s);
    struct authorizations tokens;
    init_auth(&tokens);
    if(s->ptr != NULL) {
        json_char* json = (json_char*)s->ptr;
        json_value* value = json_parse(json, s->len);
        if (value == NULL) {
            fprintf(stderr, "Unable to parse data\n");
            exit(1);
        }
        int authPos = json_contains_key(value, "access_token");
        if(authPos >= 0 && value->u.object.values[authPos].value->type == json_string) {
            char* tmp = tokens.access;
            tokens.access = concat(tokens.access, value->u.object.values[authPos].value->u.string.ptr);
            free(tmp);
        }
        int refPos = json_contains_key(value, "refresh_token");
        if(refPos >= 0 && value->u.object.values[refPos].value->type == json_string) {
            char* tmp = tokens.refresh;
            tokens.refresh = concat(tokens.refresh, value->u.object.values[refPos].value->u.string.ptr);
            free(tmp);
        }
    }
    free(s);
    free(body);
    return tokens;
}

void refreshAuthToken(char* apiKey, struct authorizations *tokens) {
    char* body = "grant_type=refresh_token&code=";
    body = concat(body, tokens->refresh);
    concat2(body, "&client_id=");
    concat2(body, apiKey);
    struct string* s;
    init_string(s);
    callApi("token", POST, "", body,s);
    if(s->ptr != NULL) {
        json_char* json = (json_char*)s->ptr;
        json_value* value = json_parse(json, s->len);
        if (value == NULL) {
          fprintf(stderr, "Unable to parse data\n");
          exit(1);
        }
        int pos = json_contains_key(value, "error_description");
        if(pos >= 0 && value->u.object.values[pos].value->type == json_string) {
          char* tmp = tokens->access;
          tokens->access = concat("", value->u.object.values[pos].value->u.string.ptr);
          free(tmp);
        }

    }
    free(s);
    free(body);
}

int main(void)
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
  char api[100]; //TODO: find exact API key length
  printf("Please enter your API key\n");
  fgets(api, sizeof(api), stdin);
  api[strcspn(api, "\r\n")] = 0;
  char* pin = getPin(api);
  if(strlen(pin) == 0) {
    printf("Failed to authenticate on step 1");
    exit(1);
  }
  printf("Access Code: %s\n", pin);
  struct authorizations tokens = getTokens(api, pin);
  if(strlen(tokens.access) == 0 || strlen(tokens.refresh) == 0) {
    printf("Failed to authenticate on step 2");
    exit(1);
  }
  printf("AccessToken: %s\n", tokens.access);
  printf("RefreshToken: %s\n", tokens.refresh);
  refreshAuthToken(api, &tokens);
  printf("New AccessToken: %s\n", tokens.access);
  curl_global_cleanup();
}
