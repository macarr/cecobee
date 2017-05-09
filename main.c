#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libs/curl/curl.h"
#include "json.h"

#define GET 1
#define POST 2
#define DELETE 3
#define PUT 4
#define PATCH 5

#define SECONDS_PER_HOUR 3600

struct string {
  char* ptr;
  size_t len;
};

typedef struct AuthTokens {
  char* access;
  char* refresh;
  time_t last_refresh;
} AuthTokens;

void init_auth(struct AuthTokens* auth) {
    auth->access = malloc(1);
    if(!auth->access) {
      fprintf(stderr, "malloc() failed\n");
      exit(EXIT_FAILURE);
    }
    auth->refresh = malloc(1);
    if(!auth->access) {
      fprintf(stderr, "malloc() failed\n");
      exit(EXIT_FAILURE);
    }
    auth->access[0] = '\0';
    auth->refresh[0] = '\0';
    auth->last_refresh = 0;
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
    printf("CONCAT: %s\n", result);
    return result;
}

char* concat2(char* s1, const char* s2) {
  s1 = realloc(s1, strlen(s1) + strlen(s2) + 1);//+1 for the zero-terminator
  if(!s1)  {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  strcat(s1, s2);
  printf("CONCAT2: %s\n", s1);
  return(s1);
}

/**
 * checks for an error in the API response
 */
int responseError(json_value* value) {
  if(json_contains_key(value, "error") >= 0) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * reports the error in the provided json_value to stdout
 */
void report_error(json_value* value) {
  int errPos = json_contains_key(value, "error");
  int errDescPos = json_contains_key(value, "error_description");
  if(errPos >= 0 && errDescPos >= 0) {
    //we will assume these are strings
    printf("ERROR> %s ::: %s\n", value->u.object.values[errPos].value->u.string.ptr,
                              value->u.object.values[errDescPos].value->u.string.ptr);
  }
}

void init_string(struct string* s) {
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

int callApi(char* endpoint, int method, char* queryParams, char* body, struct string* s) {
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
  puts("Getting PIN");
    char* queryParam = "response_type=ecobeePin&client_id=";
    queryParam = concat(queryParam, apiKey);
    queryParam = concat2(queryParam, "&scope=smartWrite");
    printf("Query params constructed: %s\n", queryParam);
    struct string s;
    init_string(&s);
    callApi("authorize", GET, queryParam, "", &s);
    if(s.ptr != NULL) {
        json_char* json = (json_char*)s.ptr;
        json_value* value = json_parse(json, s.len);
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
    free(queryParam);
    return "";
}

/**
 * step two of authorizing the app. Refresh token is for regenerating
 * authorization tokens, authorization tokens are for making the actual
 * api requests
 *
 * Returns empty tokens if the call fails
 **/
AuthTokens getTokens(char* apiKey, char* authCode) {
    char* body = "grant_type=ecobeePin&code=";
    body = concat(body, authCode);
    body = concat2(body, "&client_id=");
    body = concat2(body, apiKey);
    struct string s;
    init_string(&s);
    callApi("token", POST, "", body, &s);
    AuthTokens tokens;
    init_auth(&tokens);
    if(s.ptr != NULL) {
        json_char* json = (json_char*)s.ptr;
        json_value* value = json_parse(json, s.len);
        if (value == NULL) {
            fprintf(stderr, "Unable to parse data\n");
            exit(1);
        }
        if(!responseError(value)) {
          int authPos = json_contains_key(value, "access_token");
          if(authPos >= 0 && value->u.object.values[authPos].value->type == json_string) {
              free(tokens.access);
              tokens.access = concat("", value->u.object.values[authPos].value->u.string.ptr);
          }
          int refPos = json_contains_key(value, "refresh_token");
          if(refPos >= 0 && value->u.object.values[refPos].value->type == json_string) {
              free(tokens.access);
              tokens.refresh = concat(tokens.refresh, value->u.object.values[refPos].value->u.string.ptr);
          }
          tokens.last_refresh = time(NULL);
        } else {
          report_error(value);
        }
    }
    free(body);
    return tokens;
}

/**
 * Refresh the Access and Refresh tokens
 * The Access token expires after one hour, at which point
 * the application must request a new Access/Refresh token pair
 * using the previous Refresh token.
 *
 * Returns 0 on success, 1 if there was an error
 */
int refreshAuthToken(char* apiKey, AuthTokens *tokens) {
    char* body = "grant_type=refresh_token&code=";
    body = concat(body, tokens->refresh);
    body = concat2(body, "&client_id=");
    body = concat2(body, apiKey);
    struct string s;
    init_string(&s);
    callApi("token", POST, "", body,&s);
    if(s.ptr != NULL) {
        json_char* json = (json_char*)s.ptr;
        json_value* value = json_parse(json, s.len);
        if (value == NULL) {
          fprintf(stderr, "Unable to parse data\n");
          exit(1);
        }
        if(!responseError(value)) {
          int authPos = json_contains_key(value, "access_token");
          if(authPos >= 0 && value->u.object.values[authPos].value->type == json_string) {
              free(tokens->access);
              tokens->access = concat("", value->u.object.values[authPos].value->u.string.ptr);
          }
          int refPos = json_contains_key(value, "refresh_token");
          if(refPos >= 0 && value->u.object.values[refPos].value->type == json_string) {
              free(tokens->refresh);
              tokens->refresh = concat("", value->u.object.values[refPos].value->u.string.ptr);
          }
          tokens->last_refresh = time(NULL);
        } else {
          report_error(value);
        }
    }
    free(body);
}

int requireAuthRefresh(AuthTokens* tokens) {
  if((tokens->last_refresh + SECONDS_PER_HOUR) < time(NULL)) {
    return 1;
  } else {
    return 0;
  }
}

AuthTokens* loadAuthTokens() {
  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  ssize_t read;
  puts("Reading auth tokens from file");
  if((fp = fopen("./keys", "r"))) {
    if((read = getline(&line, &len, fp)) != -1) {
      //load tokens here
    }
  }
  fclose(fp);
  if(line)
    free(line);
  return NULL;
}

int main(void)
{
  time_t rawtime;
  struct tm* timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  printf ( "Current local time and date: %s", asctime (timeinfo) );
  char* api_key = "D88iw9CZgDGqoaJZ0PUjwbIYM11eHZD4";
  curl_global_init(CURL_GLOBAL_DEFAULT);
  printf("API Key is: %s\n", api_key);
  char* pin = getPin(api_key);
  if(strlen(pin) == 0) {
    printf("Failed to authenticate on step 1");
    exit(1);
  }
  printf("Access Code: %s\n", pin);
  AuthTokens tokens = getTokens(api_key, pin);
  if(strlen(tokens.access) == 0 || strlen(tokens.refresh) == 0) {
    printf("Failed to authenticate on step 2");
    exit(1);
  }
  printf("AccessToken: %s\n", tokens.access);
  printf("RefreshToken: %s\n", tokens.refresh);
  refreshAuthToken(api_key, &tokens);
  printf("New AccessToken: %s\n", tokens.access);
  printf("New RefreshToken: %s\n", tokens.refresh);
  curl_global_cleanup();
}
