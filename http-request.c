#include <stdio.h>
#include <string.h>
#include "http-request.h"

#define PROTOCOL "HTTP/1.0"

char* getReasonPhrase(int statusCode){
    const char* reasonPhrase = NULL;
    if ((reasonPhrase = searchByKey_Table(&HTTP_StatusCodes, statusCode)) != NULL)
        return reasonPhrase;
    else
        return "Unknown Status Code";
}

void getStatusLine(struct Request* request, char* buf){
    char* reasonPhrase = getReasonPhrase(request->statusCode);
    sprintf(buf,
        "%s %d %s\r\n",
        PROTOCOL, request->statusCode, reasonPhrase
    );
}

void setStatusCode(struct Request* request, int statusCode){
    request->statusCode = statusCode;
}

void printLog(struct Request* request) {
    char* reasonPhrase = getReasonPhrase(request->statusCode);
    fprintf(stderr, "%s \"%s %s %s\" %d %s\n", request->ipAddress,request->method, 
        request->uri, request->httpVersion, request->statusCode, reasonPhrase);
}

void checkRequest(struct Request*, char* extra);

struct Request* createRequest(const char* requestLine, char* clientIP) {
    struct Request* request = malloc(sizeof(struct Request));
    initRequest(request);
    request->ipAddress = clientIP;

    if (requestLine != NULL) {
        char* copyLine = malloc(strlen(requestLine) + 1);
        strcpy(copyLine, requestLine);
        char *token_separators = "\t \r\n"; // tab, space, new line

        request->method = strtok(copyLine, token_separators);
        request->uri = strtok(NULL, token_separators);
        request->httpVersion = strtok(NULL, token_separators);
        char *extra = strtok(NULL, token_separators);
        checkRequest(request, extra);
    }
    else {
        request->statusCode = 400;
    }
    return request;
}

int isSuccess(struct Request* request) {
    if (request->statusCode == 200)
        return 1;
    return 0;
}

void checkRequest(struct Request* request, char* extra) {

    // Shoud be only three parameters

    if (!request->method || !request->uri || !request->httpVersion || extra) {
        request->statusCode = 400; // Bad Request
        return;
    }

    // Accept only GET method

    if (strcmp("GET", request->method) != 0) {
        request->statusCode = 501; // Not Implemented
        return;
    }

    // Accept only HTTP/1.0 or HTTP/1.1

    if (strcmp("HTTP/1.0", request->httpVersion) != 0 && (strcmp("HTTP/1.1", request->httpVersion) != 0)) {
        request->statusCode = 501; // Not Implemented
        return;
    }

    // Request uri must start from "/"

    if (strncmp("/", request->uri, 1) != 0) { 
        request->statusCode = 400; // Bad Request
        return;
    }

    // Prevent "/../" for security reason
    
    if (strstr(request->uri, "/../") != NULL) {
        request->statusCode = 400; // Bad Request
        return;
    }

    // Prevent "/.." at the end of request uri

    int len = strlen(request->uri);
    if (len >= 3 && strncmp("/..", (request->uri + len - 3), 3) == 0) { 
        request->statusCode = 400; // Bad Request
        return;
    }
}
