#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include "dataStructures.h"

#define BUF_SIZE 4096 // safety concern !!!
#define MAX_BUF_SIZE 4096

const char *searchByKey_Table(struct Table* table, int key)
{
    for (int i = 0; i < table->size; i++){
        if (table->keys[i] == key) {
            return table->values[i];
        }
    }
    return NULL;
}

struct circularBuffer {
    char* array;
    size_t head;
    size_t tail;
    size_t cap;
    size_t size;
};

struct circularBuffer* init_Buffer () {
    struct circularBuffer* buffer = (struct circularBuffer*) malloc(sizeof(struct circularBuffer));
    buffer->array = (char*)malloc(sizeof(char) * BUF_SIZE);
    if (buffer->array == NULL) {
        perror("malloc failed");
        return NULL;
    }

    buffer->head = 0;
    buffer->tail = 0;
    buffer->cap = BUF_SIZE;
    buffer->size = 0; // # of bytes ready to extracted
    return  buffer;
}

static inline size_t minOf2(const size_t x, const size_t y){
    return x < y ? x : y;
}

static inline size_t optimMod (size_t n, size_t x) { // n % x, x is power of 2
    assert((x & (x - 1)) == 0);
    return n & (x - 1);
}

static inline size_t distance_head (size_t d, struct circularBuffer* buffer) { // max sector of contig memory for write, capped by d
    if (d == 0) {
        return 0;
    }
    if (buffer->size == buffer->cap) {
        return 0;
    }

    if (buffer->tail > buffer->head)
        return minOf2(d, buffer->tail - buffer->head);

    return minOf2(d, buffer->cap - buffer->head);    
}

static inline size_t distance_tail (int d, struct circularBuffer* buffer) { // max sector of contig memory for read
    if (d == 0) {
        return 0;
    }
    if (buffer->size == 0) {
        return 0;
    }

    if (buffer->head > buffer->tail)
        return minOf2(d, buffer->head - buffer->tail);

    return minOf2(d, buffer->cap - buffer->tail);
}

static int rsetCapacity(struct circularBuffer* buffer, size_t newCap) {

    char* newArray = (char*)malloc(sizeof(char) * newCap);
    if (newArray == NULL) {
        perror("rsetCapacity: malloc failed");
        return -1;
    }

    size_t first = buffer->cap - buffer->tail;
    if (first > buffer->size) {
        first = buffer->size;
    }

    memcpy(newArray, buffer->array + buffer->tail, first);
    memcpy(newArray + first, buffer->array, buffer->size - first);

    free(buffer->array);
    buffer->array = newArray;
    buffer->tail = 0;
    buffer->head = buffer->size;
    buffer->cap = newCap;
    return 0;
}

int reserve(struct circularBuffer* buffer, size_t add) {
    size_t need = buffer->size + add;
    if (need < buffer->cap) {
        return 0;
    } 
    size_t newCap = buffer->cap ? buffer->cap : BUF_SIZE;
    while (newCap < buffer->size + add) {
        newCap *= 2;
        if (newCap > MAX_BUF_SIZE) {
            fprintf(stderr, "%s\n", 
                "reserve: couldn't allocate more than MAX_BUF_SIZE");
            return -1;
        }
    }
    return rsetCapacity(buffer, newCap);
}

int isEmpty_Buffer(struct circularBuffer* buffer) {
    return (buffer->size == 0) ? 1 : 0;
}

size_t getSize_Buffer(struct circularBuffer* buffer) {
    return buffer->size;
}

int shrink (struct circularBuffer* buffer) {
    if (buffer->size == 0) {
        return rsetCapacity(buffer, BUF_SIZE);
    }

    if (buffer->size <= buffer->cap / 4) {
        size_t newCap = buffer->cap / 2;
        if (newCap < BUF_SIZE) {
            newCap = BUF_SIZE;
        }
        assert(newCap > buffer->size);
        return rsetCapacity(buffer, newCap);
    }

    return 0;
}

char at_Buffer(size_t shift, struct circularBuffer* buffer) {
    assert(shift < buffer->size);
    size_t curr = optimMod((shift + buffer->tail), buffer->cap);
    return buffer->array[curr];
}

// copy n bytes from the source to buffer
ssize_t copy_Buffer(struct circularBuffer* buffer, const void* src_, size_t n) {
    if (n == 0) {
        return 0;
    }

    size_t copy = n;

    if (n > buffer->cap - buffer->size) {
        if (reserve(buffer, n) < 0) {
            return -1;
        }
    }

    const char* src = (const char*) src_;

    // first chunk
    size_t first = distance_head(n, buffer);
    memcpy(buffer->array + buffer->head, src, first);
    buffer->head = optimMod(buffer->head + first, buffer->cap);
    buffer->size += first;
    n -= first;

    // second chunk
    if (n > 0) {
        assert(buffer->head == 0);
        size_t second = distance_head(n, buffer);
        memcpy(buffer->array + buffer->head, src + first, second);
        buffer->head = optimMod(buffer->head + second, buffer->cap);
        buffer->size += second;
        n -= second;
    }

    if (n != 0) {
        fprintf(stderr, "%s %zu instead of %zu\n", 
                "copy_Buffer: copied", copy-n, copy);
        return -1;
    }
    return copy;
}

// copy the array of strings to buffer, return number of strings copied, variadic function
size_t copyStrings_Buffer(struct circularBuffer * buffer, const char* first, ...) {
    va_list args;
    int i = 0;

    va_start(args, first);
    const char* currString = first;
    while(currString != NULL) {
        if (copy_Buffer(buffer, currString, strlen(currString)) < 0) {
            break;
        }
        currString = va_arg(args, const char*);
        i++;
    }

    va_end(args);
    return i;
}

// try to read up n bytes from file descriptor to buffer
ssize_t read_Buffer(int fd, struct circularBuffer* buffer, size_t n) {

    if (buffer->size == buffer->cap) {
        fprintf(stderr, "%s\n", "read_Buffer: buffer is full");
        return -1;
    }

    if (n > buffer->cap - buffer->size) 
        n = buffer->cap - buffer->size;

    ssize_t nread;

    while (1) {
        nread = read(fd, buffer->array + buffer->head, distance_head(n, buffer)); // limitation is here
        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("read_Buffer: read is failed");
            return -1;
        }
        else if (nread == 0){
            return 0;
        }
        else {
            size_t rv = (size_t) nread;
            buffer->head = optimMod(buffer->head + rv, buffer->cap);
            buffer->size += rv;
            assert(buffer->size <= buffer->cap);
            return nread;
        }
    }
}

// try to read up as much as possible until you’ve read min(n, space) bytes, or EOF, or error
ssize_t readn_Buffer (int fd, struct circularBuffer* buffer, size_t n) {

    if (buffer->size == buffer->cap) {
        fprintf(stderr, "%s\n", "readn_Buffer: buffer is full");
        return -1;
    }

    size_t nleft;
    if (n > buffer->cap - buffer->size) 
        nleft = buffer->cap - buffer->size;
    else
        nleft = n;

    size_t totalRead = 0;
    ssize_t nread;

    while (nleft > 0) {
        if ((nread = read(fd, buffer->array + buffer->head, distance_head(nleft, buffer))) < 0) {
            if (errno == EINTR) {
                continue;
            }
            else {
                perror("readn_Buffer: read failed");
                return -1;
            }
        }
        else if (nread == 0) {
            break; // EOF
        }

        size_t rv = (size_t) nread;
        nleft -= rv;
        totalRead += rv;
        // buffer->readInd = (buffer->readInd + nread) % buffer->cap;
        buffer->head = optimMod(buffer->head + rv, buffer->cap);
    }

    buffer->size = buffer->size + totalRead;
    return totalRead;
}

// try to write up n bytes from buffet to file descriptor
ssize_t writen_Buffer (int fd, struct circularBuffer* buffer, size_t n) {

    if (buffer->size == 0) {
        fprintf(stderr, "%s\n", "write_Buffer: buffer is empty");
        return -1;
    }

    size_t nleft;
    if (n > buffer->size) 
        nleft = buffer->size;
    else
        nleft = n;

    size_t totalWrite = 0;
    ssize_t nwrite;

    while (nleft > 0) {
        if ((nwrite = write(fd, buffer->array + buffer->tail, distance_tail(nleft, buffer))) <= 0) {
            if (nwrite < 0 && errno == EINTR) 
                nwrite = 0;
            else
                perror("write_Buffer: write failed");
                return -1;
        }

        size_t rv = (size_t) nwrite;
        nleft -= rv;
        totalWrite += rv;
        // buffer->writeInd = (buffer->writeInd + nwrite) % buffer->cap;
        buffer->tail = optimMod(buffer->tail + rv, buffer->cap);
    }

    buffer->size -= totalWrite;
    return totalWrite;
}

// transmit all remaining bytes to file pointer
void flush_Buffer(FILE* fp, struct circularBuffer* buffer){
    if (buffer->size == 0)
        return;
    if (fp == NULL) {
        fprintf(stderr, "%s\n", "flush_Buffer: file pointer is null");
    }

    // first chunk
    size_t shift = distance_tail(buffer->size, buffer);
    fwrite(buffer->array + buffer->tail, sizeof(char), shift, fp);

    buffer->tail = optimMod(buffer->tail + shift, buffer->cap);
    buffer->size -= shift;

     // second chunk
    if (buffer->size > 0) {
        assert(buffer->tail == 0);
        shift = distance_tail(buffer->size, buffer);
        fwrite(buffer->array + buffer->tail, sizeof(char), shift, fp);
        buffer->tail = optimMod(buffer->tail + shift, buffer->cap);
        buffer->size -= shift;
    }

    assert(buffer->size == 0);
    assert(buffer->tail == buffer->head);
}

/// precompute the values of prefix function for the given string
int* prefixFunction(const char* str, size_t len) {
    int* pi = malloc(sizeof(int) * (len));
    if (pi == NULL) {
        perror("malloc fail");
        exit(1);
    }
    if (len == 0) {
        free(pi);
        fprintf(stderr, "%s\n", 
            "prefixFunction receives empty string");
        return NULL;
    }
    
    pi[0] = 0; 
    
    for (int i = 1; i < len; i++) {
        int j = pi[i-1];
        while (j > 0 && str[j] != str[i]) {
            j = pi[j-1];
        }
        
        if (str[i] == str[j])
            j++;
        pi[i] = j;
    }
    
    return pi;
}

// find the first occurence of a sequence in buffer, otherwise return -1
ssize_t findSeq_Buffer(char* str, size_t len, struct circularBuffer* buffer) {
    if (len == 0) {
        return 0;
    }
    if (len > buffer->size) {
        fprintf(stderr, "%s\n", 
            "findSeq_Buffer: pattern longer than text");
        return -1;
    }
    int* pi = prefixFunction(str, len);
    if (pi == NULL) {
        return -1;
    }
    
    int q = 0; // # of characters matched
    for (int i = 0; i < buffer->size; i++) {
        while (q > 0 && str[q] != at_Buffer(i, buffer))
            q = pi[q-1];
            
        if (str[q] == at_Buffer(i, buffer))
            q++;
        if (q == len) {
            free(pi);
            assert(i + 1 - len >= 0);
            return i + 1 - len;
        }
    }
    
    free(pi);
    fprintf(stderr, "%s\n", 
            "findSeq_Buffer: no pattern found");
    return -1;
}

// guarantee to return NULL or C-string
char* getLine_Buffer(char* line, size_t n, struct circularBuffer* buffer, char* delim, size_t dlen) {
    if (n == 0) {
        fprintf(stderr, "%s\n", "getLine_Buffer: size of line is 0");
        return NULL;
    }
    if (delim == NULL || dlen == 0) {
        fprintf(stderr, "%s\n", "getLine_Buffer: delimeter is empty");
        return NULL;
    }
    if (buffer->size == 0) {
        fprintf(stderr, "%s\n", "getLine_Buffer: buffer is empty");
        return NULL;
    }

    ssize_t pos_ = findSeq_Buffer(delim, dlen, buffer);
    if (pos_ < 0) {
        return NULL;
    }

    size_t pos = (size_t) pos_;
    size_t need = pos + dlen;

    if (need + 1 > n) { // to allocate '\0'
        fprintf(stderr, "%s\n", "getLine_Buffer: not enough space to allocate line");
        return NULL;
    }

    for (size_t i = 0; i < need; i++) {
        line[i] = at_Buffer(i, buffer);
    }
    line[need] = '\0';

    buffer->tail = optimMod(buffer->tail + need, buffer->cap);
    buffer->size -= need;

    return line;
}