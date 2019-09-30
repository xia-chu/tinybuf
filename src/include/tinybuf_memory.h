#ifndef TINYBUF_MEMORY_H
#define TINYBUF_MEMORY_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

///////////////////内存相关函数指针声明/////////////////////////////
typedef void *(*malloc_ptr)(int size);
typedef void (*free_ptr)(void *ptr);
typedef void *(*realloc_ptr)(void *ptr,int size);
typedef char *(*strdup_ptr)(const char *str);

///////////////////替换内存相关的函数/////////////////////////////
void set_malloc_ptr(malloc_ptr ptr);
void set_free_ptr(free_ptr ptr);
void set_realloc_ptr(realloc_ptr ptr);
void set_strdup_ptr(strdup_ptr ptr);

///////////////////内存相关的函数/////////////////////////////
void *tinybuf_malloc(int size);
void tinybuf_free(void *ptr);
void *tinybuf_realloc(void *ptr,int size);
char *tinybuf_strdup(const char *str);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //TINYBUF_MEMORY_H
