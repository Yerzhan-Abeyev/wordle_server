#ifndef UTILITY_H
#define UTILITY_H

#include <sys/socket.h>
#include <netinet/in.h>

#define STATUS_OK 0
#define STATUS_ERROR -1
#define STATUS_CLOSE -2

typedef int status_t;

void terminate(const char* msg);

void msg(const char* msg);

void set_Address(struct sockaddr_in* , unsigned short, unsigned int);

int setReusable_Address(int);

int setBlock_Fd(int, int blocking);

void getIP_Socket(int, char*, size_t);

unsigned int getPort_Socket(int);

void* Malloc(size_t);

int Socket(int family, int type, int protocol);

int Bind(int socket, const struct sockaddr *addr, socklen_t addrLen);

int Listen(int socket, int maxNum);

int Accept(int socket, struct sockaddr *addr, socklen_t *addrlen);

ssize_t Send(int sock, const char*);

int launchListeningSocket(int);

int receiveConnectedSocket(int serverSock);

#endif 