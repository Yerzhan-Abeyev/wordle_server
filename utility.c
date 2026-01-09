#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void terminate(const char* msg) {
    perror(msg);
    exit(1);
}

int Socket(int family, int type, int protocol) {
    int rv = socket(family, type, protocol);
    if (rv < 0) {
        terminate("socket failed");
    }
    return rv;
}

int Bind(int socket, const struct sockaddr *addr, socklen_t addrLen) {
    int rv = bind(socket, addr, addrLen);
    if (rv < 0) {
        terminate("bind failed");
    }
    return rv;
}

int Listen(int socket, int maxNum) {
    int rv = listen(socket, maxNum); 
    if (rv < 0) {
        terminate("listen failed");
    }
    return rv;
}

int Accept(int socket, struct sockaddr *addr, socklen_t *addrlen) {
    int rv = accept(socket, addr, addrlen);
    if (rv < 0) {
        terminate("accept failed");
    }
    return rv;
}


