#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <errno.h>
#include <assert.h>
#include "utility.h"
#include "dataStructures.h"

#define BUF_SIZE 32
#define MAX_BUF_SIZE 4096

static inline size_t minOf2(const size_t x, const size_t y){
    return x < y ? x : y;
}

static inline size_t minOf3(const size_t x, const size_t y, const size_t z) {
    return minOf2(minOf2(x, y), z);
}

static inline size_t optimMod (size_t n, size_t x) { // n % x, x is power of 2
    assert((x & (x - 1)) == 0);
    return n & (x - 1);
}

const char *searchByKey_Table(struct Table* table, int key)
{
    for (int i = 0; i < table->size; i++){
        if (table->keys[i] == key) {
            return table->values[i];
        }
    }
    return NULL;
}

/*
 * Implementation of a FIFO queue using array. 
 * tail -> consumer extract an element
 * head -> producer insert an element
 * To optimize # of sys calls, use read & write functions insert that inser & extract n nytes instead of 1
*/

struct circularBuffer {
    char* array;
    size_t head;
    size_t tail;
    size_t cap;
    size_t size;
};

struct circularBuffer* init_Buffer () {
    struct circularBuffer* buffer = (struct circularBuffer*)malloc(sizeof(struct circularBuffer));
    buffer->array = (char*)malloc(sizeof(char) * BUF_SIZE);
    if (buffer->array == NULL) {
        perror("init_Buffer: malloc failed");
        return NULL;
    }

    buffer->head = 0;
    buffer->tail = 0;
    buffer->cap = BUF_SIZE;
    buffer->size = 0; // # of bytes ready to extracted
    return  buffer;
}

void free_Buffer(struct circularBuffer* buffer) {
    assert(buffer != NULL);
    if (buffer->array != NULL) free(buffer->array); 
    free(buffer);
}

static inline size_t distance_head (size_t d, struct circularBuffer* buffer) { // max sector of contig memory for write, capped by d
    return minOf3(d, buffer->cap - buffer->head, buffer->cap - buffer->size);
}

static inline size_t distance_tail (size_t d, struct circularBuffer* buffer) { // max sector of contig memory for read, capped by d
    return minOf3(d, buffer->cap - buffer->tail, buffer->size);
}

static inline uint8_t at_Buffer(const size_t shift, const struct circularBuffer* buffer) {
    assert(shift < buffer->size);
    size_t curr = optimMod((shift + buffer->tail), buffer->cap);
    return (uint8_t)buffer->array[curr];
}

/*
 * reallocate new memmory for buffer, 
 * return 0 on success, return -1 on error
*/
static int rsetCapacity(struct circularBuffer* buffer, size_t newCap) {
    char* newArray = (char*)malloc(sizeof(char) * newCap);
    if (newArray == NULL) {
        perror("rsetCapacity: malloc failed");
        return -1;
    }
    if (buffer->size >= newCap) {
        msg("rsetCapacity: newCap is less than buffer size");
        return -1;
    }

    size_t first = buffer->cap - buffer->tail;
    if (first > buffer->size) first = buffer->size;

    memcpy(newArray, buffer->array + buffer->tail, first);
    memcpy(newArray + first, buffer->array, buffer->size - first);

    free(buffer->array);
    buffer->array = newArray;
    buffer->tail = 0;
    buffer->head = buffer->size;
    buffer->cap = newCap;
    return 0;
}

/*
 * increase buffer memory by add bytes, 
 * return 0 on success, return -1 on error
*/
static int reserve(struct circularBuffer* buffer, const size_t add) {
    size_t need = buffer->size + add;
    if (need < buffer->cap) return 0;

    size_t newCap = buffer->cap ? buffer->cap : BUF_SIZE; // in case, if buffer->cap = 0
    assert(!(buffer->cap & (buffer->cap - 1))); // ensure buffer->cap is a power of 2
    while (newCap < buffer->size + add) {
        if (newCap > MAX_BUF_SIZE / 2) {
            msg("reserve: couldn't allocate more than MAX_BUF_SIZE");
            return -1;
        }
        newCap *= 2;
    }
    return rsetCapacity(buffer, newCap);
}

/* 
 * decrease buffer memory, if there is only a small part of used memory, 
 * return 0 on success, return -1 on error
*/
static int shrink (struct circularBuffer* buffer) {
    if (buffer->size == 0) return rsetCapacity(buffer, BUF_SIZE);

    if (buffer->size <= buffer->cap / 8) {
        size_t newCap = buffer->cap / 2;
        if (newCap < BUF_SIZE) {
            newCap = BUF_SIZE;
        }
        assert(newCap > buffer->size);
        return rsetCapacity(buffer, newCap);
    }
    return 0;
}

int isEmpty_Buffer(const struct circularBuffer* buffer) {
    assert(buffer != NULL);
    return (buffer->size == 0) ? 1 : 0;
}

size_t getSize_Buffer(const struct circularBuffer* buffer) {
    assert(buffer != NULL);
    return buffer->size;
}


/* 
 * copy n bytes from a contiguous memory struct to buffer
*/
ssize_t copy_Buffer(struct circularBuffer* buffer, const void* src_, const size_t n) {
    assert(buffer && src_);
    if (n == 0) return 0;

    size_t nleft = n;
    if (nleft > buffer->cap - buffer->size) {
        if (reserve(buffer, nleft) < 0) {
            return -1;
        }
    }

    const char* src = (const char*) src_;

    // first chunk
    size_t first = distance_head(nleft, buffer);
    memcpy(buffer->array + buffer->head, src, first);
    buffer->head = optimMod(buffer->head + first, buffer->cap);
    buffer->size += first;
    nleft -= first;

    // second chunk
    if (nleft > 0) {
        assert(buffer->head == 0);
        size_t second = distance_head(nleft, buffer);
        memcpy(buffer->array, src + first, second);
        buffer->head = optimMod(buffer->head + second, buffer->cap);
        buffer->size += second;
        nleft -= second;
    }

    if (nleft != 0) {
        fprintf(stderr, "%s %zu instead of %zu\n", 
                "copy_Buffer: copied", n - nleft, n);
        return -1;
    }
    return n;
}

/*
 * variadic function
 * copy C-strings to buffer
*/
size_t copyStrings_Buffer(struct circularBuffer* buffer, const char* first, ...) {
    assert(buffer);
    va_list args;
    int i = 0;

    va_start(args, first);
    const char* currString = first;
    while(currString != NULL) {
        if (copy_Buffer(buffer, currString, strlen(currString)) < 0) break;
        currString = va_arg(args, const char*);
        i++;
    }

    va_end(args);
    return i;
}

/*
 * copy n bytes from another buffer to buffer
*/
ssize_t copyBuffer_Buffer(struct circularBuffer* dest, struct circularBuffer* src, const size_t n) {
    if (n > dest->cap - dest->size) {
        if (reserve(dest, n) < 0) {
            return -1;
        }
    }

    size_t count = 0;
    while(!isEmpty_Buffer(src)) {
        dest->array[dest->head] = src->array[src->tail];
        dest->head = optimMod(dest->head + 1, dest->cap);
        dest->size += 1;
        src->tail = optimMod(src->tail + 1, src->cap);
        src->size -= 1;
        count++;
    }

    return count;
}

status_t read_Buffer(int fd, struct circularBuffer* buffer, size_t* n, int* error) {
    assert(n && error && buffer);
    msg("read_Buffer");

    *error = 0;
    size_t nleft = *n;
    if (nleft == 0) return STATUS_OK;

    if (nleft > buffer->cap - buffer->size) {
        if (reserve(buffer, nleft) < 0) {
            msg("read_Buffer: reserve failed");
            return STATUS_ERROR;
        }
    }

    struct iovec iov[2]; int count = 0;
    iov[0].iov_base = buffer->array + buffer->head;
    iov[0].iov_len = distance_head(nleft, buffer);
    count++;

    nleft -= iov[0].iov_len;

    if (nleft > 0) {
        iov[1].iov_base = buffer->array;
        iov[1].iov_len = nleft;
        count++;
    }

    ssize_t rv = readv(fd, iov, count);
    if (rv > 0) {
        size_t r = (size_t)rv;
        // fprintf(stderr, "recv %d", r);
        buffer->head = optimMod(r + buffer->head, buffer->cap);
        buffer->size += r;
        *n = r;
        return STATUS_OK;
    }
    else if (rv == 0) {
        return STATUS_CLOSE;
    }
    else {
        int e = errno;
        *error = e;
        perror("read_Buffer: read failed");
        return STATUS_ERROR;
    }
}

status_t write_Buffer(int fd, struct circularBuffer* buffer, size_t* n, int* error) {
    assert(n && error && buffer);
    msg("write_Buffer");

    *error = 0;
    size_t nleft = *n;
    if (nleft == 0) return STATUS_OK;

    if (isEmpty_Buffer(buffer)) {
        msg("write_Buffer: buffer is empty");
        return STATUS_ERROR;
    }
    nleft = minOf2(nleft, buffer->size);

    struct iovec iov[2]; int count = 0;
    iov[0].iov_base = buffer->array + buffer->tail;
    iov[0].iov_len = distance_tail(nleft, buffer);
    count++;

    nleft -= iov[0].iov_len;

    if (nleft > 0) {
        iov[1].iov_base = buffer->array;
        iov[1].iov_len = nleft;
        count++;
    }

    ssize_t rv = writev(fd, iov, count);
    if (rv > 0) {
        size_t r = (size_t)rv;
        fprintf(stderr, "send %d", r);
        buffer->tail = optimMod(r + buffer->tail, buffer->cap);
        buffer->size -= r;
        *n = r;
        return STATUS_OK;
    }
    else if (rv == 0) {
        return STATUS_OK;
    }
    else {
        int e = errno;
        *error = e;
        perror("write_Buffer: write failed");
        return STATUS_ERROR;
    }
}

int flush_Buffer(FILE* fp, struct circularBuffer* buffer){
    assert(fp && buffer);
    if (isEmpty_Buffer(buffer)) return 0;

    size_t first = minOf2(buffer->cap - buffer->tail, buffer->size);
    if (first != fwrite(buffer->array + buffer->tail, sizeof(char), first, fp)) {
        perror("fwrite failed");
        return -1;
    }

    size_t second = buffer->size - first;
    if (second > 0) {
        if (second != fwrite(buffer->array, sizeof(char), second, fp)) {
            perror("fwrite failed");
            return -1;
        }
    }

    buffer->tail = buffer->head; buffer->size = 0;
    return 0;
}

/* 
 * KMP algorithm
 * precompute the values of prefix function for the given string
 * return pointer to prefix array on success, return NULL on error
*/
static int* prefixFunction(const char* str, size_t len) {
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


/*
 * Dynamic Array
 * Safety concern: get_Array return a pointer to a value, not the value itself
 * Don't keep the pointer, it may become dangling after realloc(), dereference the pointer and keep the value
*/

 struct dynamicArray {
    void* array;
    size_t size; // # of elements 
    size_t elementSize; 
    size_t capacity; // # of allocated elements
 };

struct dynamicArray* init_Array(size_t cap, size_t elementSize) {
    assert(cap != 0 && elementSize != 0);

    struct dynamicArray* dArray = malloc(sizeof(struct dynamicArray));
    if (dArray == NULL) {
        perror("init_Array: malloc failed");
        return NULL;
    }
    dArray->array = malloc(cap * elementSize);
    if (dArray->array == NULL) {
        free(dArray);
        perror("init_Array: malloc failed");
        return NULL;
    }
    dArray->capacity = cap;
    dArray->elementSize = elementSize;
    dArray->size = 0;
    return dArray;
 }

 void free_Array(struct dynamicArray* dArray) {
    assert(dArray);
    free(dArray->array);
    if (dArray->array != NULL) free(dArray);
 }

 void clear_Array(struct dynamicArray* dArray) {
    assert(dArray != NULL);
    dArray->size = 0;
 }

 size_t getSize_Array(const struct dynamicArray* dArray) {
    assert(dArray != NULL);
    return dArray->size;
 }

 void* getData_Array(const struct dynamicArray* dArray) {
    assert(dArray != NULL);
    return dArray->array;
 }

 static inline int isFull_Array(struct dynamicArray* dArray) {
    return dArray->capacity == dArray->size;
 }

 void* at_Array(const struct dynamicArray* dArray, size_t index) {
    assert (dArray != NULL && index < dArray->size);
    return (uint8_t*)dArray->array + index * dArray->elementSize;
 }

 int push_Array(struct dynamicArray* dArray, const void* element) {
    assert(dArray && element);

    if (isFull_Array(dArray)) {
        size_t newCap = dArray->capacity * 2;
        void* newArray = realloc(dArray->array, newCap * dArray->elementSize);
        if (newArray == NULL) {
            perror("push_Array: realloc failed");
            return -1;
        }
        dArray->array = newArray;
        dArray->capacity *= 2;
    }
    memcpy((uint8_t*)dArray->array + dArray->size * dArray->elementSize, element, dArray->elementSize);
    dArray->size++;
    return 0;
 }

 void* pop_Array(struct dynamicArray* dArray) {
    assert (dArray->size > 0);
    dArray->size--;
    return (uint8_t*)dArray->array + dArray->size * dArray->elementSize;
 }

 void swap_Array(struct dynamicArray* dArray, size_t x, size_t y) {
    assert(dArray != NULL);
    assert(x < dArray->size && y < dArray->size);
    if (x == y) return;

    void* tmp = Malloc(dArray->elementSize);

    memcpy(tmp, at_Array(dArray, y), dArray->elementSize);
    memcpy(at_Array(dArray, y), at_Array(dArray, x), dArray->elementSize);
    memcpy(at_Array(dArray, x), tmp, dArray->elementSize);
    free(tmp);
 }
 

