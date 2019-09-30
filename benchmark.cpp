//
// Created by xzl on 2019/9/26.
//

#include "tinybuf.h"
#include "tinybuf_log.h"
#include "jsoncpp/json.h"
#include <sstream>
#include <sys/time.h>
#include <assert.h>


using namespace std;
using namespace Json;

#define JSON_COMPACT 0
#define MAX_COUNT 1 * 10000

static inline uint64_t getCurrentMicrosecondOrigin() {
#if !defined(_WIN32)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
#else
    return  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif
}

class TimePrinter {
public:
    TimePrinter(const string &str){
        _str = str;
        _start_time = getCurrentMicrosecondOrigin();
    }
    ~TimePrinter(){
        LOGD("%s占用时间:%ld ms",_str.data(),(getCurrentMicrosecondOrigin() - _start_time) / 1000);
    }

private:
    string _str;
    uint64_t _start_time;
};


tinybuf_value *tinybuf_make_test_value(){
    tinybuf_value *value = tinybuf_value_alloc();
    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_double(child,3.1415);
        tinybuf_value_map_set(value,"double",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_double(child,0.000000012345);
        tinybuf_value_map_set(value,"little double",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_map_set(value,"null",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_int(child,123456789);
        tinybuf_value_map_set(value,"+int",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_int(child,-123456789);
        tinybuf_value_map_set(value,"-int",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_bool(child,1);
        tinybuf_value_map_set(value,"bool true",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_bool(child,0);
        tinybuf_value_map_set(value,"bool false",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_string(child,"hello world string",0);
        tinybuf_value_map_set(value,"string",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        tinybuf_value_init_string(child,"",0);
        tinybuf_value_map_set(value,"empty string",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc_with_type(tinybuf_map);
        tinybuf_value_map_set(value,"empty map",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc_with_type(tinybuf_array);
        tinybuf_value_map_set(value,"empty array",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        char bytes[] = "\x1F \xAF 1234\r\nabc\t\b\f\\\"ssds\"\x00\x01\x02中文13kxsdlasdl21";
        tinybuf_value_init_string(child,bytes, sizeof(bytes));
        tinybuf_value_map_set(value,"bytes\r\n",child);
    }

    {
        tinybuf_value *child = tinybuf_value_alloc();
        char bytes[] = "bytes\r\n\x1F \xAF 1234\r\nabc\t\b\f\\\"ssds\"\x03\x01\x02中文13kxsdlasdl21";
        tinybuf_value_init_string(child,bytes, sizeof(bytes));
        //json cpp 不支持key中带'\0'
        tinybuf_value_map_set2(value,buffer_alloc2(bytes, sizeof(bytes) - 1),child);
    }


    {
        tinybuf_value *child = tinybuf_value_alloc();
        {
            tinybuf_value *array_child = tinybuf_value_alloc();
            tinybuf_value_init_double(array_child,3.1415);
            tinybuf_value_array_append(child,array_child);
        }

        {
            tinybuf_value *array_child = tinybuf_value_alloc();
            tinybuf_value_init_int(array_child,123456789);
            tinybuf_value_array_append(child,array_child);
        }

        {
            tinybuf_value *array_child = tinybuf_value_alloc();
            tinybuf_value_init_int(array_child,-123456789);
            tinybuf_value_array_append(child,array_child);
        }

        {
            tinybuf_value *array_child = tinybuf_value_alloc();
            tinybuf_value_init_bool(array_child,1);
            tinybuf_value_array_append(child,array_child);
        }

        {
            tinybuf_value *array_child = tinybuf_value_alloc();
            tinybuf_value_init_bool(array_child,0);
            tinybuf_value_array_append(child,array_child);
        }

        tinybuf_value_array_append(child,tinybuf_value_clone(child));
        tinybuf_value_array_append(child,tinybuf_value_clone(value));
        tinybuf_value_map_set(value,"array",child);
    }

    tinybuf_value_map_set(value,"map",tinybuf_value_clone(value));
    return value;
}


void tinybuf_value_printf(const tinybuf_value *value){
    buffer *out = buffer_alloc();
    tinybuf_value_serialize_as_json(value, out,JSON_COMPACT);
    LOGI("\r\n%s",buffer_get_data(out));
    buffer_free(out);
}

tinybuf_value *deserialize_from_jsoncpp(const char *json){
    Value obj;
    stringstream ss(json);
    ss >> obj;
    string new_json = obj.toStyledString();

    tinybuf_value *ret = tinybuf_value_alloc();
    tinybuf_value_deserialize_from_json(new_json.data(),new_json.size(),ret);
    return ret;
}


void tinybuf_value_test(){
    tinybuf_value *value_origin = tinybuf_make_test_value();
    tinybuf_value *value_deserialize = tinybuf_value_alloc();
    tinybuf_value *value_deserialize_from_json = tinybuf_value_alloc();

    buffer *buf = buffer_alloc();

    tinybuf_value_serialize(value_origin,buf);
    tinybuf_value_deserialize(buffer_get_data(buf),buffer_get_length(buf),value_deserialize);

    buffer_set_length(buf,0);
    tinybuf_value_serialize_as_json(value_origin,buf,JSON_COMPACT);
    tinybuf_value_deserialize_from_json(buffer_get_data(buf),buffer_get_length(buf),value_deserialize_from_json);
    tinybuf_value *value_deserialize_from_jsoncpp = deserialize_from_jsoncpp(buffer_get_data(buf));

    assert(tinybuf_value_is_same(value_origin,value_deserialize));
    assert(tinybuf_value_is_same(value_origin,value_deserialize_from_json));
    assert(tinybuf_value_is_same(value_origin,value_deserialize_from_jsoncpp));

    tinybuf_value_printf(value_origin);

    tinybuf_value_free(value_origin);
    tinybuf_value_free(value_deserialize);
    tinybuf_value_free(value_deserialize_from_json);
    tinybuf_value_free(value_deserialize_from_jsoncpp);
    buffer_free(buf);
}

int main(int argc,char *argv[]){
    tinybuf_value_test();

    //几米对象
    tinybuf_value *value = tinybuf_make_test_value();
    //jimi格式序列化的数据
    buffer *buf_binary = buffer_alloc();
    //json格式序列化的数据
    buffer *buf_json = buffer_alloc();

    //几米序列化
    tinybuf_value_serialize(value,buf_binary);
    //json序列化
    tinybuf_value_serialize_as_json(value,buf_json,JSON_COMPACT);


    {
        //测试几米序列化性能
        TimePrinter printer("序列化成tinybuf");
        buffer *buf = buffer_alloc();
        for(int i = 0 ; i < MAX_COUNT; ++i ){
            buffer_set_length(buf,0);
            tinybuf_value_serialize(value,buf);
        }
        buffer_free(buf);
    }

    {
        //测试几米序列化成json性能
        TimePrinter printer("序列化成json格式");
        buffer *buf = buffer_alloc();
        for(int i = 0 ; i < MAX_COUNT; ++i ){
            buffer_set_length(buf,0);
            tinybuf_value_serialize_as_json(value,buf,JSON_COMPACT);
        }
        buffer_free(buf);
    }

    {
        //测试json列化性能
        TimePrinter printer("jsoncpp序列化");
        //json对象
        Value obj;
        stringstream ss(buffer_get_data(buf_json));
        ss >> obj;
        for(int i = 0 ; i < MAX_COUNT; ++i ){
            obj.toStyledString();
        }
    }

    {
        //测试几米反序列化性能
        TimePrinter printer("tinybuf反序列化");
        tinybuf_value *value_deserialize = tinybuf_value_alloc();
        for(int i = 0 ; i < MAX_COUNT; ++i ){
            tinybuf_value_clear(value_deserialize);
            tinybuf_value_deserialize(buffer_get_data(buf_binary),buffer_get_length(buf_binary),value_deserialize);
        }
        tinybuf_value_free(value_deserialize);

    }

    {
        //测试几米反序列json化性能
        TimePrinter printer("json反序列化");
        tinybuf_value *value_deserialize = tinybuf_value_alloc();
        for(int i = 0 ; i < MAX_COUNT; ++i ){
            tinybuf_value_clear(value_deserialize);
            tinybuf_value_deserialize_from_json(buffer_get_data(buf_json),buffer_get_length(buf_json),value_deserialize);
        }
        tinybuf_value_free(value_deserialize);
    }

    {
        //测试json反序列化性能
        TimePrinter printer("jsoncpp反序列化");
        Value obj;
        Json::Reader reader;
        string str(buffer_get_data(buf_json));
        for(int i = 0 ; i < MAX_COUNT; ++i ){
            reader.parse(str, obj);
        }
    }

    buffer_free(buf_binary);
    buffer_free(buf_json);
    tinybuf_value_free(value);
    return 0;
}