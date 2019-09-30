#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tinybuf_memory.h"
#include "tinybuf_log.h"

static malloc_ptr s_malloc_ptr = NULL;
static free_ptr s_free_ptr = NULL;
static realloc_ptr s_realloc_ptr = NULL;
static strdup_ptr s_strdup_ptr = NULL;


void set_malloc_ptr(malloc_ptr ptr){
    s_malloc_ptr = ptr;
}

void set_free_ptr(free_ptr ptr){
    s_free_ptr = ptr;
}

void set_realloc_ptr(realloc_ptr ptr){
    s_realloc_ptr = ptr;
}

void set_strdup_ptr(strdup_ptr ptr){
    s_strdup_ptr = ptr;
}

void *tinybuf_malloc(int size){
    assert(size);
    void *ptr = s_malloc_ptr ? s_malloc_ptr(size) : malloc(size);
    assert(ptr);
    return ptr;
}

void tinybuf_free(void *ptr){
    if(ptr == NULL){
        return;
    }
    s_free_ptr ? s_free_ptr(ptr) : free(ptr);
}

void *tinybuf_realloc(void *ptr,int size){
    assert(ptr);
    assert(size);
    void *ret = s_realloc_ptr ? s_realloc_ptr(ptr,size) : realloc(ptr,size);
    assert(ret);
    return ret;
}

static inline char *strdup_1(const char *str){
    int len = strlen(str) + 1;
    char *ret = tinybuf_malloc(len);
    memcpy(ret,str,len);
    assert(ret);
    return ret;
}

char *tinybuf_strdup(const char *str){
    assert(str);
    char *ret = s_strdup_ptr ? s_strdup_ptr(str) : strdup_1(str);
    assert(ret);
    return ret;
}