#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>

/* Standard Functions APIs */

void *my_malloc(size_t size);
void *my_calloc(size_t n, size_t size);
void  my_free(void *ptr);
void *my_realloc(void *p, size_t new_size);

/* Multithreaded Arguments Structs */

typedef struct my_malloc_args {
    size_t size;
    void* result;
} malloc_args;


typedef struct my_realloc_args {
    void* p;
    size_t new_size;
    void* result;
} realloc_args;


typedef struct my_free_args {
    void* p;
} free_args;

typedef struct my_calloc_args {
    size_t num;
    size_t size;
    void* result;
} calloc_args;

/* Multithreaded Functions APIs */

void* twrap_my_malloc(void* args);
void* twrap_my_realloc(void* args);
void* twrap_my_free(void* args);
void* twrap_my_calloc(void* args);

#endif