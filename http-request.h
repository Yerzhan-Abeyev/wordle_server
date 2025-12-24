#include <stdlib.h>

#define MAX_TABLE_SIZE 100

struct Request {
    char* method;
    char* uri;
    char* httpVersion;
    int statusCode;
    char* ipAddress;
};

static inline void initRequest(struct Request* request)
{
    request->method = "";
    request->uri = "";
    request->httpVersion = "";
    request->statusCode = 200;
}

void setStatusCode(struct Request*, int);

void getStatusLine(struct Request*, char*);

void printLog(struct Request*);

struct Request* createRequest(const char*, char*);

int isSuccess(struct Request*);

struct Table{
    int keys[MAX_TABLE_SIZE]; 
    char* values[MAX_TABLE_SIZE];
    int size;
};

static inline const char *searchByKey(struct Table* table, int key)
{
    for (int i = 0; i < table->size; i++){
        if (table->keys[i] == key) {
            return table->values[i];
        }
    }
    return NULL;
}

static struct Table HTTP_StatusCodes = {
    .keys = {200, 201, 202, 204, 301, 302, 304, 400, 401, 403, 404, 500, 501, 502, 503},
    .values = {
        "OK",
        "Created",
        "Accepted",
        "No Content",
        "Moved Permanently",
        "Moved Temporarily",
        "Not Modified",
        "Bad Request",
        "Unauthorized",
        "Forbidden",
        "Not Found",
        "Internal Server Error",
        "Not Implemented",
        "Bad Gateway",
        "Service Unavailable"
    },
    .size = MAX_TABLE_SIZE
};

char* getReasonPhrase(int);
