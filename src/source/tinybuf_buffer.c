#include <memory.h>
#include <assert.h>
#include "tinybuf_buffer.h"
#include "tinybuf_memory.h"
#include "tinybuf_buffer_private.h"

#define RESERVED_SIZE 64


static inline void my_memcpy(char *dst,const char *src,int len){
    assert(dst);
    assert(src);
    switch (len){
        case 0:
            return;
        case 1:
            dst[0] = *src;
            return;
        default:
            memcpy(dst,src,len);
            return;
    }
}

static inline int buffer_release(buffer *buf){
    assert(buf);
    if(buf->_capacity && buf->_data){
        tinybuf_free(buf->_data);
        //LOGD("free:%d",buf->_capacity);
    }
    memset(buf,0, sizeof(buffer));
    return 0;
}

buffer *buffer_alloc(void){
    buffer *ret = (buffer *) tinybuf_malloc(sizeof(buffer));
    assert(ret);
    memset(ret,0, sizeof(buffer));;
    return ret;
}

buffer *buffer_alloc2(const char *str,int len){
    buffer *ret = buffer_alloc();
    buffer_assign(ret,str,len);
    return ret;
}


int buffer_free(buffer *buf){
    assert(buf);
    buffer_release(buf);
    tinybuf_free(buf);
    return 0;
}


char *buffer_get_data(buffer *buf){
    if(!buf){
        return NULL;
    }
    return buf->_data;
}

int buffer_get_length(buffer *buf){
    if(!buf){
        return 0;
    }
    return buf->_len;
}

int buffer_set_length(buffer *buf, int len){
    if(!buf){
        return -1;
    }
    if(buf->_capacity <= len){
        return -1;
    }

    buf->_len = len;
    buf->_data[len] = '\0';
    return 0;
}

int buffer_get_capacity(buffer *buf){
    if(!buf){
        return 0;
    }
    return buf->_capacity;
}

int buffer_append_buffer(buffer *buf,const buffer *from){
    assert(buf);
    assert(from);
    return buffer_append(buf,from->_data,from->_len);
}

int buffer_add_capacity(buffer *buf,int len){
    assert(buf);
    if(!buf->_capacity){
        //内存尚未开辟
        buf->_data = tinybuf_malloc(len);
        buf->_capacity = len;
        return 0;
    }
    buf->_data = tinybuf_realloc(buf->_data,len + buf->_capacity);
    buf->_capacity += len;
    return 0;
}

int buffer_append(buffer *buf,const char *data,int len){
    assert(buf);
    assert(data);
    if(len <= 0){
        len = strlen(data);
    }

    if(!buf->_capacity){
        //内存尚未开辟
        buffer_add_capacity(buf,len + RESERVED_SIZE);
    }

    if(buf->_capacity > buf->_len + len){
        //容量够
        my_memcpy(buf->_data + buf->_len ,data,len);
        buf->_len += len;
        buf->_data[buf->_len] = '\0';
        return 0;
    }

    //已经开辟的容量不够
    buf->_data = tinybuf_realloc(buf->_data,len + buf->_len + RESERVED_SIZE);
    assert(buf->_data);
    my_memcpy(buf->_data + buf->_len ,data,len);
    buf->_len += len;
    buf->_data[buf->_len] = '\0';
    buf->_capacity = buf->_len + RESERVED_SIZE;
    return 0;
}
int buffer_assign(buffer *buf,const char *data,int len){
    assert(buf);
    assert(data);
    if(len <= 0){
        len = strlen(data);
    }
    if(buf->_capacity > len){
        my_memcpy(buf->_data,data,len);
        buf->_len = len;
        buf->_data[buf->_len] = '\0';
        return 0;
    }
    buf->_len = 0;
    return buffer_append(buf,data,len);
}

int buffer_move(buffer *dst,buffer *src){
    buffer_release(dst);
    dst->_data = src->_data;
    dst->_len = src->_len;
    dst->_capacity = src->_capacity;
    memset(src,0, sizeof(buffer));
    return 0;
}

static int buffer_length(buffer *buf){
    if(!buf){
        return 0;
    }
    return buf->_len;
}

int buffer_push(buffer *buf,char ch){
    assert(buf);
    if(buf->_len + 1 < buf->_capacity){
        buf->_data[buf->_len] = ch;
        buf->_len += 1;
        buf->_data[buf->_len] = '\0';
    }else{
        buffer_append(buf,&ch,1);
    }
    return 0;
}

int buffer_pop(buffer *buf){
    assert(buf);
    if(!buf->_len){
        return 0;
    }
    buf->_data[buf->_len - 1] = '\0';
    buf->_len -= 1;
    return 0;
}

char buffer_back(buffer *buf){
    assert(buf);
    if(buf->_len){
        return buf->_data[buf->_len - 1];
    }
    return 0;
}

int buffer_is_same(const buffer *buf1,const buffer *buf2){
    assert(buf1);
    assert(buf2);
    
    if(buf1 == buf2){
        return 1;
    }

    if(buf1->_len != buf2->_len){
        //长度不一致
        return 0;
    }
    return 0 == memcmp(buf1->_data,buf2->_data,buf1->_len);
}