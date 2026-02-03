#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <stdlib.h>

#include "dataStructures.h"
#include "network.h"
#include "utility.h"

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

status_t handle_Request(struct Connection* c);

#endif
