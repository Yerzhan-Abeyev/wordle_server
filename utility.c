#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/inet.h>

void terminate(const char* msg) {
    perror(msg);
    exit(1);
}

void msg(const char* msg) {
    fprintf(stderr, "%s\n", msg);
}


void set_Address(struct sockaddr_in* serverAddr, 
    unsigned short serverPort, unsigned int serverIP) {
    
    serverAddr->sin_family = AF_INET;                     // internet address family
    serverAddr->sin_port = htons(serverPort);             // port
    serverAddr->sin_addr.s_addr = htonl(serverIP);        // ip address
}

int setReusable_Address(int socket) {
    int val = 1;
    socklen_t len;
    len = sizeof(val);

    /* set the SET_REUSEADDR option to value 1*/
    return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &val, len);
}

int setBlock_Fd(int fd, int blocking) {
    if (fd < 0) {
        msg("file descriptor is negative");
        return -1;
    }
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl failed");
        return -1;
    }
    flags = blocking ? (flags | ~O_NONBLOCK) : (flags | O_NONBLOCK);
    int r = fcntl(fd, F_SETFL, flags);
    if (r < 0) {
        perror("fcntl failed");
        return -1;
    }
    return 1;
}

void getIP_Socket(int fd, char* ip, size_t size) {
    assert(ip && fd >= 0);
    assert(size >= 16);

    struct sockaddr_in my_addr = {};
    socklen_t len = sizeof(my_addr);

    getsockname(fd, (struct sockaddr *) &my_addr, &len);
    inet_ntop(AF_INET, &my_addr.sin_addr, ip, size);
}

unsigned int getPort_Socket(int fd) {
    assert(fd >= 0);

    unsigned int port;
    struct sockaddr_in my_addr = {};
    socklen_t len = sizeof(my_addr);
    getsockname(fd, (struct sockaddr *) &my_addr, &len);
    port = ntohs(my_addr.sin_port);

    return port;
}

void* Malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        msg("malloc failed");
    }
    return ptr;
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

ssize_t Send(int sock, const char* buf) { 
    size_t len = strlen(buf);
    ssize_t res = send(sock, buf, len, 0);
    if (res != len) {
        perror("send failed");
        return -1;
    }
    else 
        return res;
}


int launchListeningSocket(int serverPort) {
    /* AF_INET (PF_INET) is for IPv4, SOCK_STREM is for TCP, 0 (IPPROTO_TCP) */
    int serverSock = Socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
    setReusable_Address(serverSock);

    struct sockaddr_in serverAddr = {};
    set_Address(&serverAddr, serverPort, INADDR_ANY);
    socklen_t addrLen = sizeof(serverAddr);

    Bind(serverSock, (struct sockaddr*) &serverAddr, addrLen);
    Listen(serverSock, SOMAXCONN);
    return serverSock;
}

int receiveConnectedSocket(int serverSock) {
    // create socket for connection
    struct sockaddr_in clientAddr = {};
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSock = Accept(serverSock, (struct sockaddr *) &clientAddr, &clientLen);
    return clientSock;
}

