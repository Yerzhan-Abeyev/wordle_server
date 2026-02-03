#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <sys/types.h>
#include "utility.h"
#include "dataStructures.h"
#define MAX_TABLE_SIZE 100

struct Table{
    int keys[MAX_TABLE_SIZE]; 
    char* values[MAX_TABLE_SIZE];
    int size;
};

const char *searchByKey_Table(struct Table*, int);

struct circularBuffer;

struct circularBuffer* init_Buffer();

void free_Buffer(struct circularBuffer*);

int isEmpty_Buffer(const struct circularBuffer*);

size_t getSize_Buffer(const struct circularBuffer*);

/* 
 * copy n bytes from the source to buffer
 * return n on succes, return -1 on error
*/
ssize_t copy_Buffer(struct circularBuffer*, const void*, const size_t);

/*
 * variadic function
 * copy the array of strings to buffer
 * return number of strings copied
*/
size_t copyStrings_Buffer(struct circularBuffer *, const char*, ...);

/*
 *
*/
ssize_t copyBuffer_Buffer(struct circularBuffer*, struct circularBuffer*, const size_t);

/*
 * value-result argument
 * try to read n bytes from file descriptor to buffer
 * return STATUS_OK, STATUS_CLOSE, STATUS_ERROR
*/
status_t read_Buffer (int, struct circularBuffer*, size_t*, int*);

/*
 * value-result argument
 * try to write n bytes from buffer to file descriptor 
 * return STATUS_OK, STATUS_ERROR
*/
status_t write_Buffer (int, struct circularBuffer*, size_t*, int*);

/*
 * transmit all remaining bytes to file pointer
 * return 0 on success, return -1 on error
*/
int flush_Buffer(FILE*, struct circularBuffer*);

/*
 * return the first occurence of a sequence in buffer, otherwise return -1
*/
ssize_t findSeq_Buffer(char*, size_t, struct circularBuffer*);

/*
 * return substring that ended by delimeter (e.g \n)
 * guarantee to return NULL or C-string
*/
char* getLine_Buffer(char*, size_t, struct circularBuffer*, char*, size_t);

struct dynamicArray;

struct dynamicArray* init_Array(size_t, size_t);

void free_Array(struct dynamicArray*);

void clear_Array(struct dynamicArray*);

size_t getSize_Array(const struct dynamicArray*);

void* getData_Array(const struct dynamicArray*);

void* at_Array(const struct dynamicArray*, size_t);

int push_Array(struct dynamicArray*, const void*);

void* pop_Array(struct dynamicArray*);

void swap_Array(struct dynamicArray*, size_t, size_t);

#endif