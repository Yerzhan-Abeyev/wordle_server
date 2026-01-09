#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <sys/types.h>
#define MAX_TABLE_SIZE 100

struct Table{
    int keys[MAX_TABLE_SIZE]; 
    char* values[MAX_TABLE_SIZE];
    int size;
};

const char *searchByKey_Table(struct Table*, int);

struct circularBuffer;

struct circularBuffer* init_Buffer();

int isEmpty_Buffer(struct circularBuffer*);

size_t getSize_Buffer(struct circularBuffer*);

ssize_t copy_Buffer(struct circularBuffer*, const void*, size_t);

size_t copyStrings_Buffer(struct circularBuffer *, const char*, ...);

ssize_t read_Buffer (int, struct circularBuffer*, size_t);

ssize_t readn_Buffer (int, struct circularBuffer*, size_t);

ssize_t writen_Buffer (int, struct circularBuffer*, size_t);

void flush_Buffer(FILE*, struct circularBuffer*);

ssize_t findSeq_Buffer(char*, size_t, struct circularBuffer*);

char* getLine_Buffer(char*, size_t, struct circularBuffer*, char*, size_t);

#endif