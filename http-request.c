#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "stdio.h"
#include "http-request.h"
#include "game.h"
#include "network.h"
#include "dataStructures.h"
#include "utility.h"

#define PROTOCOL "HTTP/1.1"
#define LINE_SIZE 512
#define BUF_SIZE 1024

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

void handle_Error(struct Request* request, struct Connection* c) {
    char buf[BUF_SIZE];
    printLog(request);
    getStatusLine(request, buf);
    Send(c->fd, buf);

    Send(c->fd, "\r\n");
    sprintf(buf,
            "<html><body>\n"
            "<h1>%d %s</h1>\n"
            "</body></html>\n",
            request->statusCode, getReasonPhrase(request->statusCode)
    );
    Send(c->fd, buf);
}

void handle_Headers(struct Request* request, struct Connection* c) {
    assert(request && c);

    char* endOfLine = "\r\n";
    char line[LINE_SIZE];
    while (!isEmpty_Buffer(c->incoming) && getLine_Buffer(line, sizeof(line), c->incoming, endOfLine, strlen(endOfLine)) != NULL) {
        fprintf(stderr, "%s", line);
        if (strcmp(line, endOfLine) == 0)
            return;
    }

    setStatusCode(request, 400);
}

void handle_Wordle(struct Request* request, struct Connection* c) {
    msg("handle_Wordle");

    const char *form =
        "<html><body>\n"
        "<h1>wordle</h1>\n"
        "<p>\n"
        "<form method=GET action=/wordle>\n"
        "lookup: <input type=text name=key>\n"
        "<input type=submit>\n"
        "</form>\n"
        "<p>\n"
        ;

    char headers[LINE_SIZE];

    char statusLine[LINE_SIZE];
    getStatusLine(request, statusLine);

    char* gameUriKey = "/wordle?key=";
    char* wordleUri = "/wordle";
    if (strncmp(request->uri, gameUriKey, strlen(gameUriKey)) == 0) {

        //extract wordle word

        const char* key = request->uri + strlen(gameUriKey);

        // request to game 
        
        int rv = check(key);
        char* wordleTable;
        wordleTable = screen(rv); // need to add flag when game is finished

        // headers

        snprintf(headers, sizeof(headers), 
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %lu\r\n"
            "Connection: keep-alive\r\n", 
            strlen(wordleTable));
        
        //response to browser

        if (copyStrings_Buffer(c->outgoing, statusLine, headers, "\r\n", wordleTable, NULL) != 4){
            msg("handle_Wordle: copyStrings_Buffer failed");
            setStatusCode(request, 500);
            return;
        }

        write_Connection(c);
    }
    else if (strcmp(request->uri, wordleUri) == 0){

        // start game

        start();

        // headers

        snprintf(headers, sizeof(headers), 
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %lu\r\n"
            "Connection: keep-alive\r\n", 
            strlen(form));

        //response to browser

        if (copyStrings_Buffer(c->outgoing, statusLine, headers, "\r\n", form, NULL) != 4){
            msg("handle_Wordle: copyStrings_Buffer failed");
            setStatusCode(request, 500);
            return;
        }

        write_Connection(c);
    }
    else {
        setStatusCode(request, 400);
    }
}

status_t handle_Request(struct Connection* c) {
    char line[LINE_SIZE];
    char* endOfLine = "\r\n";

    int fd = c->fd;
    msg("I am here - 3");

    // get request line
    if (getLine_Buffer(line, sizeof(line), c->incoming, endOfLine, strlen(endOfLine)) == NULL) {
        msg("handle_httpRequest: getLine_Buffer failed");
        return STATUS_ERROR;
    }
    else {
        char ipAddr[16];
        getIP_Socket(fd, ipAddr, sizeof(ipAddr));
            
        struct Request* request = createRequest(line, ipAddr);
        if (!isSuccess(request)) {
            handle_Error(request, c);
            return STATUS_CLOSE;
        }

        // handle headers

        handle_Headers(request, c);
        if (!isSuccess(request)) {
            handle_Error(request, c); 
            return STATUS_CLOSE;
        }

        // handle wordle request 

        char *gameURI = "/wordle";

        if (strncmp(request->uri, gameURI, strlen(gameURI)) == 0){
            handle_Wordle(request, c);
        }
        else {
            ;
            // handleFileRequest(request, c);
        }

        if (isSuccess(request)) {
            return STATUS_OK;
        } 
        else {
            handle_Error(request, c); 
            return STATUS_CLOSE;
        }
    }
}
