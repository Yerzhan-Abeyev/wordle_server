#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <poll.h> // for poll() and pollfd
#include <sys/stat.h> // for stat() and IFDIR

#include <arpa/inet.h> // for inet_ntop

#include "dataStructures.h"
#include "http-request.h"
#include "network.h"
#include "utility.h"

#define BUF_SIZE 4096
#define LINE_SIZE 1000
#define CONNECTIONS_SIZE 100


#define MAX_HEADERS_SIZE 1024
#define HEADER_TIMEOUT_MS 50


int main(int argc, char** argv) {
    if (argc != 3){
        fprintf(stderr, "usage: %s <server_port> <web_root>", argv[0]);
    }
    unsigned short serverPort = atoi(argv[1]); // atoi is not safe
    char* webRoot = argv[2];

    // parallel arrays for Connections and pollfds

    struct dynamicArray* connections = init_Array(CONNECTIONS_SIZE, sizeof(struct Connection*));
    struct dynamicArray* pollfd_array = init_Array(CONNECTIONS_SIZE, sizeof(struct pollfd));

    // create listening connection

    int serverSock = launchListeningSocket(serverPort);
    setBlock_Fd(serverSock, 0);

    struct Connection* listenConnection = create_Connection(serverSock, 1, 0, 0, NULL);
    if (listenConnection == NULL) msg("main: create_Connection failed");
    push_Array(connections, &listenConnection);

    while(1) {

        // prepare the event loop

        clear_Array(pollfd_array);
        size_t i = 0;
        while (1) {
            if (i < getSize_Array(connections)) {
                break;
            }

            struct Connection* conn = *((struct Connection**)at_Array(connections, i));
            if (conn->fd == -1) {
                free(conn);
                swap_Array(connections, i, getSize_Array(connections) - 1);
                pop_Array(connections);
            }
            else {
                i++;
            }
        }

        // upload pollfds for connections

        size_t numOfConn = getSize_Array(connections);
        for (int i = 0; i < numOfConn; i++) { 
            struct Connection* c = *((struct Connection**)at_Array(connections, i));
            struct pollfd pfd = {};
            assign_pollfd(c, &pfd);
            push_Array(pollfd_array, &pfd);
        }

        // blocking poll

        int rv = poll((struct pollfd*)getData_Array(pollfd_array), (nfds_t)getSize_Array(pollfd_array), -1);
        if (rv < 0 && errno == EINTR) {
            continue;
        }
        if (rv < 0) {
            terminate("poll failed");
        }

        // process ready connections

        for (size_t i = 0; i < numOfConn; i++) {

            struct pollfd pfd = *((struct pollfd*)at_Array(pollfd_array, i));
            struct Connection* conn = *((struct Connection**)at_Array(connections, i));
            assert(conn->fd == pfd.fd);

            if (pfd.revents & POLLIN) {
                if (conn->to_listen) {
                    listen_Connection(conn, connections, 0);
                }
                else if (conn->to_read) {
                    read_Connection(conn);
                }
            }

            if (pfd.revents & POLLOUT) {
                if (conn->to_write) {
                    if (!isEmpty_Buffer(conn->incoming)) process(conn);
                }
            }

            if ((pfd.revents & POLLERR) || conn->to_close) {
                if (pfd.revents & POLLERR) {
                    int error = 0;
                    socklen_t errlen = sizeof(error);
                    getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, (void *)&error, &errlen);
                }

                close_Connection(conn);
                msg("close connection");
            }
        }
    }

  return 0;
}



/*
 while(1) {

        if (!isEmpty_Buffer(buffC2S)) {
            flush_Buffer(stdout, buffC2S);
        }

        struct sockaddr_in clientAddr = {};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSock = Accept(serverSock, (struct sockaddr *) &clientAddr, &clientLen);

        // get request line

        if (read_Buffer(clientSock, buffC2S, STREAM_SIZE) < 0) {
            exit(1);
        }

        char line[LINE_SIZE];
        char* end = "\r\n";
        if (getLine_Buffer(line, sizeof(line), buffC2S, end, strlen(end)) == NULL) {
            exit(1);
        }

        char* clientIP = inet_ntoa(clientAddr.sin_addr); // get client ip address
        struct Request* request = createRequest(line, clientIP);
        if (!isSuccess(request)) {
            handleError(request, clientSock); // client error
            continue; 
        }

        // handle header files

        handleHeaders(request, buffC2S, clientSock);
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

        if (isSuccess(request)) {
            close(clientSock);
        }
    }
*/

/*


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
    Send(clientSock, buf);

    // printLog(request);
    close(clientSock);
}


*/

