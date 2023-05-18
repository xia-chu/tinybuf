#ifndef TINYBUF_BUFFER_H
#define TINYBUF_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct T_buffer buffer;

/**
 * 在堆上申请开辟buffer对象并且初始化
 * 内部会调用buffer_init函数
 * @return buffer对象
 */
buffer *buffer_alloc(void);
buffer *buffer_alloc2(const char *str,int len);

/**
 * 释放堆上的buffer对象
 * @param buf buffer对象指针
 * @return 0:成功
 */
int buffer_free(buffer *buf);

/**
 * 获取数据指针
 * @param buf 对象
 * @return 数据指针，可能返回NULL
 */
char *buffer_get_data(buffer *buf);

/**
 * 获取数据长度
 * @param buf 对象
 * @return 数据长度
 */
int buffer_get_length(buffer *buf);

/**
 * 设置长度，不能超过(容量-1)
 * @param buf 对象
 * @param len 新的长度
 * @return 0成功，-1失败
 */
int buffer_set_length(buffer *buf,int len);

/**
 * 获取对象容量
 * @param buf 对象
 * @return 容量大小
 */
int buffer_get_capacity(buffer *buf);

/**
 * 追加数据至buffer对象末尾
 * @param buf 对象指针
 * @param data 数据指针
 * @param len 数据长度，可以为0，为0时内部使用strlen获得长度
 * @return 0为成功
 */
int buffer_append(buffer *buf,const char *data,int len);

/**
 * 追加容量
 * @param buf 对象
 * @param add 追加的字节数
 * @return
 */
int buffer_add_capacity(buffer *buf,int add);

/**
 * 追加数据至buffer对象末尾
 * @param buf 对象指针
 * @param from 子buffer
 * @return 0为成功
 */
int buffer_append_buffer(buffer *buf,const buffer *from);

/**
 * 复制数据至buffer对象
 * @param buf 对象指针
 * @param data 数据指针
 * @param len 数据长度，可以为0，为0时内部使用strlen获得长度
 * @return 0为成功
 */
int buffer_assign(buffer *buf,const char *data,int len);

/**
 * 把src对象里面的数据移动至dst，内部有无memcpy
 * @param dst 目标对象
 * @param src 源对象
 * @return 0为成功
 */
int buffer_move(buffer *dst,buffer *src);

/**
 * 压入一个字节
 * @param buf
 * @param ch
 * @return
 */
int buffer_push(buffer *buf,char ch);

/**
 * 弹出一个字节
 * @param buf
 * @return
 */
int buffer_pop(buffer *buf);

/**
 * 获取最后一个字节
 * @param buf
 * @return
 */
char buffer_back(buffer *buf);

/**
 * 判断两个对象是否一致
 * @param buf
 * @param from
 * @return
 */
int buffer_is_same(const buffer *buf1,const buffer *buf2);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus
#endif //TINYBUF_BUFFER_H
