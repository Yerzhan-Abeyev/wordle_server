#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <ctype.h>  
#include "http-request.h"
#include "game.h"

#define BUF_SIZE 4096
#define LINE_SIZE 1000
#define PROTOCOL "HTTP/1.0"

static void terminate(const char* msg) {
    perror(msg);
    exit(1);
}

static int createServerSocket() {

    // obtain socket file descriptor

    /* AF_INET (PF_INET) is for IPv4, SOCK_STREM is for TCP, 0 (IPPROTO_TCP) */
    int serverSock = socket(AF_INET, SOCK_STREAM, 0); 
    if (serverSock < 0){
        terminate("socket failed");
    }

    // set socket options

    int val = 1;
    /* set the SET_REUSEADDR option to value 1, so we can reuse port number after restart */
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    
    return serverSock;
}

static void setSocketAddress(struct sockaddr_in *serverAddr, 
    unsigned short serverPort, unsigned int serverIP) {
    
    // fill sockaddr_in struct
    
    serverAddr->sin_family = AF_INET;                     // internet address family
    serverAddr->sin_port = htons(serverPort);             // port
    if (serverIP == 0)
        serverAddr->sin_addr.s_addr = htonl(INADDR_ANY);  // use all IP addresses of the machine
    else 
        serverAddr->sin_addr.s_addr = htonl(serverIP);    // ip address
}

static void Bind(int serverSock, const struct sockaddr_in *serverAddr) {
    socklen_t serverLen = sizeof(*serverAddr);
    int rv = bind(serverSock, (struct sockaddr *)serverAddr, serverLen);
    if (rv < 0) {
        terminate("bind failed");
    }
}

static void Listen(int serverSock) {
    int rv = listen(serverSock, SOMAXCONN); // SOMAXCONN is 4096 on Linux
    if (rv < 0) {
        terminate("listen failed");
    }
}

ssize_t Send(int sock, const char *buf) { // ssize_t is [-1, SSIZE_MAX]
    size_t len = strlen(buf);
    ssize_t res = send(sock, buf, len, 0);
    if (res != len) {
        perror("send failed");
        return -1;
    }
    else 
        return res;
}

int createPath(const char* rootPath, const char* filePath, char* fullPath) {
    if (filePath[strlen(filePath) - 1] == '/')
        sprintf(fullPath, "%s%sindex.html", rootPath, filePath);
    else {
        sprintf(fullPath, "%s%s", rootPath, filePath);

        // if the path is directory, expect "/" at the end

        struct stat sb;
        if (stat(fullPath, &sb) == 0) {
            if (S_ISDIR(sb.st_mode))
                return -1;
        }
    }
    return 0;
}

void handleError(struct Request* request, int clientSock) {
    char buf[BUF_SIZE];
    printLog(request);
    getStatusLine(request, buf);
    Send(clientSock, buf);

    Send(clientSock, "\r\n");
    sprintf(buf,
            "<html><body>\n"
            "<h1>%d %s</h1>\n"
            "</body></html>\n",
            request->statusCode, getReasonPhrase(request->statusCode)
    );

    // printLog(request);
    close(clientSock);
}

int handleFileRequest(struct Request* request, char* webRoot, int clientSock){

    // restore full path

    char* fullPath = malloc(strlen(webRoot) + strlen(request->uri) + 100);
    if (fullPath == NULL)
        terminate("malloc failed"); // server error
    
    if (createPath(webRoot, request->uri, fullPath) == -1){
        setStatusCode(request, 400); // Bad Request
        handleError(request, clientSock);
        free(fullPath);
        return -1;
    }

    // open file
    
    fprintf(stderr, "%s", fullPath);
    FILE* fp = fopen(fullPath, "rb");
    if (fp == NULL) {
        setStatusCode(request, 404);
        handleError(request, clientSock);
        free(fullPath);
        return -1;
    }

    // send response to browser

    char buf[BUF_SIZE];
    getStatusLine(request, buf);
    Send(clientSock, buf);
    Send(clientSock, "\r\n");

    // read and send file

    int r;
    while ((r = fread(buf, 1, sizeof(buf), fp)) > 0) {
        if (send(clientSock, buf, r, 0) != r) {
            perror("send failed");
            break;
        }
    }

    printLog(request);
    fclose(fp); free(fullPath);
    return 0;
}


int handleWordleRequest(struct Request* request, char* webRoot, int clientSock) {
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
    char buf[BUF_SIZE];
    getStatusLine(request, buf);

    char* gameUriKey = "/wordle?key=";
    char* wordleUri = "/wordle";
    if (strncmp(request->uri, gameUriKey, strlen(gameUriKey)) == 0) {

        //extract wordle word

        const char* key = request->uri + strlen(gameUriKey);

        // request to game 
        
        int rv = check(key);
        char* wordleTable;
        wordleTable = screen(rv); // need to add flag when game is finished
        
        //response to browser

        Send(clientSock, buf);
        Send(clientSock, "\r\n");
        Send(clientSock, wordleTable);
    }
    else if (strcmp(request->uri, wordleUri) == 0){

        // start game

        start();

        //response to browser

        Send(clientSock, buf);
        Send(clientSock, "\r\n");
        Send(clientSock, form); // just send request form
    }
    else {
        setStatusCode(request, 400);
        handleError(request, clientSock);
        return -1;
    }

    printLog(request);
    return 0;
}


int main(int argc, char** argv) {
    if (argc != 3){
        fprintf(stderr, "usage: %s <server_port> <web_root>", argv[0]);
    }
    unsigned short serverPort = atoi(argv[1]); // atoi is not safe
    char* webRoot = argv[2];

    int serverSock = createServerSocket();
    struct sockaddr_in serverAddr = {};
    setSocketAddress(&serverAddr, serverPort, 0);
    Bind(serverSock, &serverAddr);
    Listen(serverSock);

    // accept incoming connections
    char line[LINE_SIZE];
    
    while(1) {
        struct sockaddr_in clientAddr = {};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSock < 0) {
            terminate("accept failed"); // server error
        }

        /* file wrapper */
        FILE* input = fdopen(clientSock, "rb");
        if (input < 0) {
            terminate("fdopen failed"); // server error
        }

        // get request line

        if (fgets(line, sizeof(line), input) == NULL) {
            if (ferror(input)){
                terminate("fdopen failed"); // server error
            }
        }

        char* clientIP = inet_ntoa(clientAddr.sin_addr); // get client ip address
        struct Request* request = createRequest(line, clientIP);
        if (!isSuccess(request)) {
            handleError(request, clientSock); // client error
            continue; 
        }

        // skip header files

        while(1) {
            if (fgets(line, sizeof(line), input) == NULL) {
                if (ferror(input)){
                    terminate("fdopen failed"); // server error
                }
                else{
                    setStatusCode(request, 400); // Bad Request
                    break;
                }
            }
            if (strcmp(line, "\r\n") || strcmp(line, "\n"))
                break;
        }
        if (!isSuccess(request)) {
            handleError(request, clientSock); // client error
            continue; 
        }


        // handle request 

        char *gameURI = "/wordle";

        if (strncmp(request->uri, gameURI, strlen(gameURI)) == 0){
            handleWordleRequest(request, webRoot, clientSock);
        }
        else {
            handleFileRequest(request, webRoot, clientSock);
        }

        close(clientSock);
    }

  return 0;
}
