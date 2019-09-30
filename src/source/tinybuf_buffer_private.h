#ifndef TINYBUF_BUFFER_PRIVATE_H
#define TINYBUF_BUFFER_PRIVATE_H

typedef struct buffer{
    char *_data;
    int _len;
    int _capacity;
} buffer;

#define inline_optimization 1

#if inline_optimization
#define buffer_get_data_inline(buf) (buf ? buf->_data : NULL)
#define buffer_get_length_inline(buf) (buf ? buf->_len : 0)
#define buffer_get_capacity_inline(buf) (buf ? buf->_capacity : 0)
#define buffer_push_inline(buf,ch)\
do{\
        assert(buf);\
        if(buf->_len + 1 < buf->_capacity){\
            buf->_data[buf->_len] = ch;\
            buf->_len += 1;\
            buf->_data[buf->_len] = '\0';\
        }else{\
            char tmp = ch;\
            buffer_append(buf,&tmp,1);\
        }\
    }while (0);
#else
#define buffer_get_data_inline buffer_get_data
#define buffer_get_length_inline buffer_get_length
#define buffer_get_capacity_inline buffer_get_capacity
#define buffer_push_inline buffer_push
#endif

#endif //TINYBUF_BUFFER_PRIVATE_H
