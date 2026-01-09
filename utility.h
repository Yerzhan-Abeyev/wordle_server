#ifndef UTILITY_H
#define UTILITY_H

#include <sys/socket.h>

void terminate(const char* msg);

int Socket(int family, int type, int protocol);

int Bind(int socket, const struct sockaddr *addr, socklen_t addrLen);

int Listen(int socket, int maxNum);

int Accept(int socket, struct sockaddr *addr, socklen_t *addrlen);

#endif 