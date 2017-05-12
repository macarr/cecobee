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

#define API_KEY "D88iw9CZgDGqoaJZ0PUjwbIYM11eHZD4"

#define KEYFILE "./keys"

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

AuthTokens currentTokens;


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
 * pull Authtokens from the keyfile
 */
int loadAuthTokens(AuthTokens* tokens) {
  FILE* fp;
  char* line = NULL;
  size_t len = 0;
  int pos = 0;
  ssize_t read;
  puts("Reading auth tokens from file");
  if((fp = fopen(KEYFILE, "r"))) {
    while((read = getline(&line, &len, fp)) != -1) {
      //Line1 is the access token, Line2 is the refresh token, Line3 is the last refresh time
      line[strcspn(line, "\n")] = 0;
      if(strlen(line) == 0) {
        return 1;
      }
      switch(pos) {
      case 0:
        tokens->access = concat("", line);
        break;
      case 1:
        tokens->refresh = concat("", line);
        break;
      case 2:
        tokens->last_refresh = atoi(line);
        break;
      default:
        break;
      }
      pos++;
    }
    if(pos < 3) {
      puts("Keyfile corrupted, a value was missing");
      return 1;
    }
  } else {
    puts("Keyfile does not exist");
    return 1;
  }
  fclose(fp);
  if(line)
    free(line);
  return 0;
}

/**
 * store Authtokens in the keyfile
 */
void persistAuthTokens(AuthTokens* tokens) {
  FILE* fp;
  if((fp = fopen(KEYFILE, "w"))) {
    fprintf(fp, "%s\n", tokens->access);
    fprintf(fp, "%s\n", tokens->refresh);
    fprintf(fp, "%d\n", (int)tokens->last_refresh);
  } else {
    fclose(fp);
    if((fp = fopen("rescue", "w"))) {
      puts("Error creating keyfile, attempting to rescue");
      fprintf(fp, "%s\n", tokens->access);
      fprintf(fp, "%s\n", tokens->refresh);
      fprintf(fp, "%d\n", (int)tokens->last_refresh);
    } else {
      puts("Critical error: rescue failed. You will need to re-pair your application online");
      fclose(fp);
    }
  }
  fclose(fp);
}

int requireAuthRefresh(AuthTokens* tokens) {
  if(difftime(time(NULL), tokens->last_refresh) > SECONDS_PER_HOUR) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * step one of authorizing the app
 **/
char* getPin() {
    puts("Getting PIN");
    char* queryParam = "response_type=ecobeePin&client_id=";
    queryParam = concat(queryParam, API_KEY);
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
void getTokens(char* authCode, AuthTokens* tokens) {
    puts("Using PIN to retrieve Authorizations");
    char* body = "grant_type=ecobeePin&code=";
    body = concat(body, authCode);
    body = concat2(body, "&client_id=");
    body = concat2(body, API_KEY);
    struct string s;
    init_string(&s);
    callApi("token", POST, "", body, &s);
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
              tokens->access = concat("", value->u.object.values[authPos].value->u.string.ptr);
          }
          int refPos = json_contains_key(value, "refresh_token");
          if(refPos >= 0 && value->u.object.values[refPos].value->type == json_string) {
              tokens->refresh = concat("", value->u.object.values[refPos].value->u.string.ptr);
          }
          tokens->last_refresh = time(NULL);
        } else {
          report_error(value);
        }
    }
    persistAuthTokens(tokens);
    free(body);
}

/**
 * Refresh the Access and Refresh tokens
 * The Access token expires after one hour, at which point
 * the application must request a new Access/Refresh token pair
 * using the previous Refresh token.
 *
 * Returns 0 on success, 1 if there was an error
 */
int refreshAuthToken(AuthTokens *tokens) {
    puts("Refreshing Authorizations");
    char* body = "grant_type=refresh_token&code=";
    body = concat(body, tokens->refresh);
    body = concat2(body, "&client_id=");
    body = concat2(body, API_KEY);
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
    persistAuthTokens(tokens);
    free(body);
}

int main(void)
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
  printf("API Key is: %s\n", API_KEY);
  AuthTokens tokens;
  if(loadAuthTokens(&tokens) != 0) {
    char* pin = getPin();
    if(strlen(pin) == 0) {
      printf("Failed to authenticate on step 1");
      exit(1);
    }
    printf("Access Code: %s\n", pin);
    getTokens(pin, &tokens);
    if(strlen(tokens.access) == 0 || strlen(tokens.refresh) == 0) {
      printf("Failed to authenticate on step 2");
      exit(1);
    }
  }
  printf("AccessToken: %s\n", tokens.access);
  printf("RefreshToken: %s\n", tokens.refresh);
  if(requireAuthRefresh(&tokens) == 1) {
    refreshAuthToken(&tokens);
    printf("New AccessToken: %s\n", tokens.access);
    printf("New RefreshToken: %s\n", tokens.refresh);
  }
  curl_global_cleanup();
}
