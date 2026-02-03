#ifndef NETWORK_H
#define NETWORK_H

struct Connection {
    int fd; 
    struct circularBuffer* incoming;
    struct circularBuffer* outgoing;
    int to_listen; int to_read; int to_write; int to_close;
    size_t dataStream_size;
    char* protocol;
};

struct Connection* create_Connection(int fd, int listen, int read, int write, char* protocol);

status_t init_Connection(struct Connection* c, int fd, int listen, int read, int write, char* protocol);

void deinit_Connection(struct Connection* c);

void free_Connection(struct Connection* c);

/*
 * close the file descriptor and deinitialize connection
*/
void close_Connection(struct Connection* c);

/*
 * accept new connections from listening socket
*/
void listen_Connection(struct Connection* c, struct dynamicArray* connections, int blocking);

/*
 * read from connected socket, append to incoming buffer
*/
void read_Connection(struct Connection* c);

/*
 * write to connected socket, extract from outgoing buffer
*/
void write_Connection(struct Connection* c);

/*
 *
*/
void process(struct Connection* c);

struct pollfd* assign_pollfd(struct Connection* c, struct pollfd* pfd);

#endif
