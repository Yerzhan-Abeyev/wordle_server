
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h> // for close
#include <string.h> // fro strcmpy
#include <poll.h>

#include "dataStructures.h"
#include "http-request.h"
#include "utility.h"

#define DATA_STREAM_SIZE 4096
#define LINE_SIZE 512
// Protocols: HTTP, echo, file

struct Connection* create_Connection(int fd, int listen, int read, int write, char* protocol) {
    struct Connection* c = (struct Connection*)Malloc(sizeof(struct Connection));
    if (c == NULL) return NULL;

    if (fd < 0) {
        msg("create_connection: file descriptor is not valid");
        return NULL;
    }
    c->fd = fd; 
    c->to_listen = listen;
    c->to_read = read;
    c->to_write = write;
    c->protocol = protocol;

    if (read || write) {
        c->incoming = init_Buffer();
        if (c->incoming == NULL) {
            msg("create_Connection: init_Buffer failed");
            free(c);
            return NULL;
        }

        c->outgoing = init_Buffer();
        if (c->outgoing == NULL) {
            msg("create_Connection: init_Buffer failed");
            free(c);
            return NULL;
        }
    
        c->dataStream_size = DATA_STREAM_SIZE;
    }

    return c;
}

status_t init_Connection(struct Connection* c, int fd, int listen, int read, int write, char* protocol) {
    assert(c);

    if (fd < 0) {
        msg("init_connection: file descriptor is not valid");
        return STATUS_ERROR;
    }
    c->fd = fd;
    c->to_listen = listen;
    c->to_read = read;
    c->to_write = write;
    c->protocol = protocol;

    if (read || write) {
        c->incoming = init_Buffer();
        if (c->incoming == NULL) {
            msg("create_Connection: init_Buffer failed");
            return STATUS_ERROR;
        }

        c->outgoing = init_Buffer();
        if (c->outgoing == NULL) {
            msg("create_Connection: init_Buffer failed");
            return STATUS_ERROR;
        }
    
        c->dataStream_size = DATA_STREAM_SIZE;
    }

    return STATUS_OK;
}

void deinit_Connection(struct Connection* c) {
    assert(c);
    c->fd = -1; 

    if (c->incoming != NULL) {
        free_Buffer(c->incoming);
    }
    if (c->outgoing != NULL) {
        free_Buffer(c->outgoing);

    c->to_read = 0; c->to_write = 0; c->to_close = 0;
    c->dataStream_size = 0; c->protocol = NULL;
    }
}

void free_Connection(struct Connection* c) {
    deinit_Connection(c);
    free(c);
}

void close_Connection(struct Connection* c) {
    assert(c && c->to_close);

    close(c->fd);
    deinit_Connection(c);
}

void listen_Connection(struct Connection* c, struct dynamicArray* connections, int blocking) {
    assert(c && connections);

    msg("new connection");

    int clientFd = receiveConnectedSocket(c->fd);
    if (setBlock_Fd(clientFd, blocking) <  0) {
        msg("listenConnection: setBlock_Socket failed");
        return;
    }

    // here, must be login part
    ;

    struct Connection* conn = create_Connection(clientFd, 0, 1, 1, "http");
    if (conn == NULL) {
        msg("listen_Connection: create_Connection failed");
        return;
    }
    push_Array(connections, &conn);
}

void read_Connection(struct Connection* c) {
    assert(c && c->to_read);

    int error = 0;
    while(1) {
        size_t n = c->dataStream_size;
        status_t status = read_Buffer(c->fd, c->incoming, &n, &error);

        if (status == STATUS_OK) return;
        else if (status == STATUS_CLOSE) {
            msg("read_Connection: client closed");
            c->to_close = 1;
            return;
        }
        else {
            if (error == EINTR) continue;
            else if (error == EAGAIN || error == EWOULDBLOCK) return;
            msg("read_Connectio: read_Buffer failed");
            c->to_close = 1;
            return;
        }
    }
}

void write_Connection(struct Connection* c) {
    assert(c && c->to_write);

    if (isEmpty_Buffer(c->outgoing)) {
        return;
    }

    int error = 0;
    while(1) {
        size_t n = c->dataStream_size;
        status_t status = write_Buffer(c->fd, c->outgoing, &n, &error);
        msg("finish write_Buffer");
        if (status == STATUS_OK) return;
        else {
            if (error == EINTR) continue;
            else if (error == EAGAIN || error == EWOULDBLOCK) return;
            msg("write_Connection: write_Buffer failed");
            c->to_close = 1;
            return;
        }
    }
}

void process(struct Connection *c) {
    assert(c);
    msg("process connection");

    if (strcmp(c->protocol, "echo") == 0) {
        if (!isEmpty_Buffer(c->incoming)) {
            size_t len = getSize_Buffer(c->incoming);
            if (copyBuffer_Buffer(c->outgoing, c->incoming, len) != len) {
                msg("process: copyBuffer_Buffer failed");
                return;
            }
            
            assert(isEmpty_Buffer(c->incoming));
            write_Connection(c);
        }
    }
    else if (strcmp(c->protocol, "http") == 0) {
        char* end = "\r\n\r\n";
        if (findSeq_Buffer(end, strlen(end), c->incoming) == -1) {
            msg("process: waiting for completing http request");
            return;
        }
        else {
            if (handle_Request(c) == STATUS_CLOSE) {
                close_Connection(c);
            }
        }
    }
}

struct pollfd* assign_pollfd(struct Connection* c, struct pollfd* pfd) {
    assert(c != NULL && pfd != NULL);

    pfd->events = 0; pfd->revents = 0;
    pfd->fd = c->fd; 

    if (c->to_listen || c->to_read) {
        pfd->events |= POLLIN;
    }
    if (c->to_write) {
        pfd->events |= POLLOUT;
    }

    return pfd;
}
