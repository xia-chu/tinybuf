#ifndef TINYBUF_H
#define TINYBUF_H
#include <stdint.h>
#include "tinybuf_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum {
    tinybuf_null = 0,
    tinybuf_int,
    tinybuf_bool,
    tinybuf_double,
    tinybuf_string,
    tinybuf_map,
    tinybuf_array
} tinybuf_type;


typedef struct tinybuf_value tinybuf_value;

/**
 * 创建对象
 * @return 返回对象
 */
tinybuf_value *tinybuf_value_alloc();


/**
 * 创建对象
 * @return 返回对象
 */
tinybuf_value *tinybuf_value_alloc_with_type(tinybuf_type type);

/**
 * 销毁对象
 * @param value 对象
 * @return
 */
int tinybuf_value_free(tinybuf_value *value);

/**
 * 重置对象为null类型
 * @param value 对象
 * @return
 */
int tinybuf_value_clear(tinybuf_value *value);

/**
 * 复制对象
 * @param value 原对象
 * @return
 */
tinybuf_value *tinybuf_value_clone(const tinybuf_value *value);

/**
 * 比较两个对象是否一致
 * @param value1 对象1
 * @param value2 对象2
 * @return 1代表一致，0不一致
 */
int tinybuf_value_is_same(const tinybuf_value *value1,const tinybuf_value *value2);

/**
 * 获取数据类型
 * @param value 对象
 * @return 数据类型
 */
tinybuf_type tinybuf_value_get_type(const tinybuf_value *value);

/**
 * 获取int值
 * @param value 对象
 * @return int值
 */
int64_t tinybuf_value_get_int(const tinybuf_value *value);

/**
 * 获取double值
 * @param value 对象
 * @return double值
 */
double tinybuf_value_get_double(const tinybuf_value *value);

/**
 * 读取bool值
 * @param value 对象
 * @return bool值
 */
int tinybuf_value_get_bool(const tinybuf_value *value);

/**
 * 读取string值
 * @param value 对象
 * @return string值
 */
buffer* tinybuf_value_get_string(const tinybuf_value *value);

/**
 * 获取array或map类型时的成员个数
 * @param value 对象
 * @return 成员个数
 */
int tinybuf_value_get_child_size(const tinybuf_value *value);

/**
 * 获取成员
 * @param value 对象
 * @param index 索引
 * @return 成员对象的指针
 */
const tinybuf_value* tinybuf_value_get_array_child(const tinybuf_value *value,int index);

/**
 * 根据key获取成员对象
 * @param value 对象
 * @param key 键
 * @return 成员对象
 */
const tinybuf_value* tinybuf_value_get_map_child(const tinybuf_value *value,const char *key);
const tinybuf_value* tinybuf_value_get_map_child2(const tinybuf_value *value,const char *key, int key_len);

/**
 * 获取map成员对象以及key
 * @param value 对象
 * @param index 索引
 * @param key key指针的指针
 * @return 成员对象的指针
 */
const tinybuf_value* tinybuf_value_get_map_child_and_key(const tinybuf_value *value, int index,buffer **key);
////////////////////////////////赋值////////////////////////////////

/**
 * 对象赋值为int类型
 * @param value 对象
 * @param int_val 整数
 * @return
 */
int tinybuf_value_init_int(tinybuf_value *value, int64_t int_val);

/**
 * 对象赋值为bool型
 * @param value 对象
 * @param flag bool值
 * @return
 */
int tinybuf_value_init_bool(tinybuf_value *value, int flag);

/**
 * 对象赋值为double类型
 * @param value 对象
 * @param db_val 双精度浮点型
 * @return
 */
int tinybuf_value_init_double(tinybuf_value *value, double db_val);

/**
 * 对象赋值为string类型
 * @param value 对象
 * @param str 字符串
 * @param str_len 字符串长度
 * @return
 */
int tinybuf_value_init_string(tinybuf_value *value, const char *str, int str_len);

/**
 * 无拷贝式赋值string
 * @param value 对象
 * @param buf 被转移所有权的对象，调用该函数后不能再buffer_free
 * @return
 */
int tinybuf_value_init_string2(tinybuf_value *value, buffer *buf);


/**
 * 对象转换成map并添加键值对
 * @param parent 对象
 * @param key 键
 * @param value 被转移所有权的value子对象,调用该函数后不能再tinybuf_value_free
 * @return
 */
int tinybuf_value_map_set(tinybuf_value *parent, const char *key, tinybuf_value *value);

/**
 * 对象转换成map并添加键值对
 * @param value 被转移所有权的value子对象,调用该函数后不能再tinybuf_value_free
 * @param key 被转移所有权的key对象，调用该函数后不能再buffer_free
 * @return
 */
int tinybuf_value_map_set2(tinybuf_value *parent,buffer *key,tinybuf_value *value);

/**
 * 对象转换成array并添加成员
 * @param parent 对象
 * @param value 被转移所有权的value子对象,调用该函数后不能再tinybuf_value_free
 * @return
 */
int tinybuf_value_array_append(tinybuf_value *parent, tinybuf_value *value);


////////////////////////////////序列化////////////////////////////////

/**
 * 对象序列化成字节
 * @param value 对象
 * @param out 字节流存放地址
 * @return
 */
int tinybuf_value_serialize(const tinybuf_value *value, buffer *out);

/**
 * 对象序列化成json字节流
 * @param value 对象
 * @param out 字节流存放地址
 * @param compact 是否紧凑(去除多余的空格回车符等)
 * @return
 */
int tinybuf_value_serialize_as_json(const tinybuf_value *value, buffer *out,int compact);

////////////////////////////////反序列化////////////////////////////////

/**
 * 反序列化对象
 * @param ptr 字节流
 * @param size 字节流长度
 * @param out 输出对象，必须不为空
 * @return 序列化消耗的字节数
 */
int tinybuf_value_deserialize(const char *ptr, int size, tinybuf_value *out);

/**
 * 加载value部分
 * @param ptr json字符串
 * @param size json字符串长度
 * @param out vlue值输出
 * @return 消耗的字节数
 */
int tinybuf_value_deserialize_from_json(const char *ptr, int size, tinybuf_value *out);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif //TINYBUF_H
