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

#define BUF_SIZE 4096
#define PROTOCOL "HTTP/1.0"
#define HOSTNAME_LEN 256

static int createServerSocket() {

  // Obtain socket file descriptor

  /* AF_INET is for IPv4, SOCK_STREM is for TCP */
  int serverSock = socket(AF_INET, SOCK_STREAM, 0); 
  if (serverSock < 0){
    perror("socket failed");
    exit(1);
  }

  // Set socket options

  int val = 1;
  /* set the SET_REUSEADDR option to value 1, so we can reuse port number after restart */
  setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  
  return serverSock;
}

static void setSocketAddress(struct sockaddr_in *serverAddr, unsigned short serverPort, unsigned int serverIP) {
  
  // Fill sockaddr_in struct
  
  serverAddr.sin_family = AF_INET;                 // internet address family
  serverAddr.sin_port = htons(serverPort);         // port
  if (serverIP == 0)
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // use all IP addresses of the machine
  else 
    serverAddr.sin_addr.s_addr = htonl(serverIP);
}

static void Bind(int serverSock, const struct sockaddr_in *serverAddr) {
  socklen_t servLen = sizeof(serverAddr);
  int rv = bind(servSock, (struct sockaddr *)&servAddr, servLen);
  if (rv < 0) {
    perror("bind failed");
    exit(1);
  }
}

int main(int argc, char** argv) {

  int serverSock = createServerSocket();
  struct sockaddr_in serverAddr = {};
  setSocketAddress(&serverAddr, serverPort, 0);
  Bind(serverSock, *serverAddr);

  // Accept incoming connections
  
  while(1) {
    struct sockaddr_in clntAddr = {};
    socklen_t clntLen = sizeof(clntAddr);
    int clntSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clntLen);
    if (clntSock < 0) {
      perror("accept failed");
      exit(1);
    }
  }

  return 0;
}

