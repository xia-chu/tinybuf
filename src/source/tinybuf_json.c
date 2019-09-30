#include "tinybuf_private.h"

//////////////////////////////////////json序列化//////////////////////////////////////
//预留的空格
static char s_blank[] = "                                                                                    ";
static void add_blank(buffer *out,int blank){
    if(!blank){
        return;
    }
    if(blank > sizeof(s_blank)){
        char *blanks = tinybuf_malloc(blank);
        memset(blanks,' ',blank);
        buffer_append(out,blanks,blank);
        tinybuf_free(blanks);
    }else{
        buffer_append(out,s_blank,blank);
    }
}

static inline char to_char(uint8_t ch){
    if(ch < 10){
        return '0' + ch;
    }
    if (ch < 16){
        return 'A' + (ch - 10);
    }
    assert(0);
    return '0';
}

static inline int dump_binary(buffer *out,uint8_t ch){
    buffer_push_inline(out,to_char(ch >> 4));
    buffer_push_inline(out,to_char(ch & 0x0F));
    return 2;
}

static inline int isControlCharacter(char ch) { return ch > 0 && ch <= 0x1F; }

static inline void json_encode_string(buffer *out, uint8_t *in, int len){
    if(len <= 0){
        len = strlen((char *)in);
    }
    int i;
    for(i = 0 ; i < len ; ++i){
        char ch = in[i];
        switch (ch){
            case '\"':
                buffer_append(out,"\\\"",2);
                break;
            case '\\':
                buffer_append(out,"\\\\",2);
                break;
            case '\b':
                buffer_append(out,"\\b",2);
                break;
            case '\f':
                buffer_append(out,"\\f",2);
                break;
            case '\n':
                buffer_append(out,"\\n",2);
                break;
            case '\r':
                buffer_append(out,"\\r",2);
                break;
            case '\t':
                buffer_append(out,"\\t",2);
                break;
            default:{
                if ((isControlCharacter(ch)) || (ch == 0)) {
                    buffer_append(out,"\\u00",4);
                    dump_binary(out,ch);
                } else {
                    buffer_append(out,&ch,1);
                }
            }
                break;
        }
    }
}

static inline void dump_double(double value,buffer *out){
    // Allocate a buffer that is more than large enough to store the 16 digits of
    // precision requested below.
    char buffer[32];
    int len = -1;

// Print into the buffer. We need not request the alternative representation
// that always has a decimal point because JSON doesn't distingish the
// concepts of reals and integers.
#if defined(_MSC_VER) && defined(__STDC_SECURE_LIB__) // Use secure version with
    // visual studio 2005 to
                                                      // avoid warning.
#if defined(WINCE)
  len = _snprintf(buffer, sizeof(buffer), "%.17g", value);
#else
  len = sprintf_s(buffer, sizeof(buffer), "%.17g", value);
#endif
#else
    if (isfinite(value)) {
        len = snprintf(buffer, sizeof(buffer), "%.17g", value);
    } else {
        // IEEE standard states that NaN values will not compare to themselves
        if (value != value) {
            len = snprintf(buffer, sizeof(buffer), "null");
        } else if (value < 0) {
            len = snprintf(buffer, sizeof(buffer), "-1e+9999");
        } else {
            len = snprintf(buffer, sizeof(buffer), "1e+9999");
        }
        // For those, we do not need to call fixNumLoc, but it is fast.
    }
#endif
    assert(len >= 0);
    buffer_append(out,buffer,len);
}

typedef struct{
    buffer *out;
    int i;
    int size;
    int compact;
    int level;
} for_each_context;

static int tinybuf_value_serialize_as_json_level(int level,int compact, const tinybuf_value *value, buffer *out);

static int avl_tree_for_each_node_dump_array(void *user_data,AVLTreeNode *node){
    for_each_context *context = (for_each_context *)user_data;
    tinybuf_value *val = (tinybuf_value *)avl_tree_node_value(node);
    if(val->_type != tinybuf_map && val->_type != tinybuf_array && !context->compact){
        add_blank(context->out,4 * context->level + 4);
    }
    //写value
    tinybuf_value_serialize_as_json_level(context->level + 1,context->compact, val, context->out);
    if(context->i++ != context->size - 1){
        buffer_push_inline(context->out,',');
    }
    if(!context->compact) {
        buffer_append(context->out, "\r\n", 2);
    }

    //继续遍历
    return 0;
}

static int avl_tree_for_each_node_dump_map(void *user_data,AVLTreeNode *node){
    for_each_context *context = (for_each_context *)user_data;
    buffer *key = (buffer*)avl_tree_node_key(node);
    tinybuf_value *val = (tinybuf_value *)avl_tree_node_value(node);
    if(!context->compact) {
        add_blank(context->out, 4 * context->level + 4);
    }
    buffer_append(context->out,"\"",1);
    json_encode_string(context->out, (uint8_t *) buffer_get_data_inline(key), buffer_get_length_inline(key));
    if(!context->compact){
        buffer_append(context->out,"\" : ",4);
    }else{
        buffer_append(context->out,"\":",2);
    }
    if((val->_type == tinybuf_map || val->_type == tinybuf_array) && !context->compact){
        buffer_append(context->out,"\r\n",2);
    }
    //写value
    tinybuf_value_serialize_as_json_level(context->level + 1,context->compact, val, context->out);

    if(context->i++ != context->size - 1){
        buffer_push_inline(context->out,',');
    }
    if(!context->compact){
        buffer_append(context->out,"\r\n",2);
    }

    //继续遍历
    return 0;
}

enum {
    /// Constant that specify the size of the buffer that must be passed to
    /// uintToString.
    uintToStringBufferSize = 3 * sizeof(int64_t) + 1
};


// Defines a char buffer for use with uintToString().
typedef char UIntToStringBuffer[uintToStringBufferSize];

/** Converts an unsigned integer to string.
 * @param value Unsigned interger to convert to string
 * @param current Input/Output string buffer.
 *        Must have at least uintToStringBufferSize chars free.
 */
static inline void uintToString(int64_t value, char** current) {
    *--(*current) = 0;
    do {
        *--(*current) = (signed char)(value % 10U + (unsigned)('0'));
        value /= 10;
    } while (value != 0);
}

static inline int dump_int(buffer *out,int64_t value){
    UIntToStringBuffer buffer;
    char* current = buffer + sizeof(buffer);
    if (value < 0) {
        uintToString(-value, &current);
        *--current = '-';
    } else {
        uintToString(value, &current);
    }
    buffer_append(out,current,0);
    return 0;
}
static int tinybuf_value_serialize_as_json_level(int level,int compact, const tinybuf_value *value, buffer *out){
    assert(value);
    assert(out);
    switch (value->_type){
        case tinybuf_null:{
            buffer_append(out,"null",4);
        }
            break;

        case tinybuf_int:{
            dump_int(out,value->_data._int);
        }
            break;

        case tinybuf_bool:{
            buffer_append(out,value->_data._bool ? "true" : "false",0);
        }
            break;

        case tinybuf_double:{
            dump_double(value->_data._double,out);
        }
            break;

        case tinybuf_string:{
            buffer_append(out,"\"",1);
            int len = buffer_get_length_inline(value->_data._string);
            if(len){
                json_encode_string(out, (uint8_t *) buffer_get_data_inline(value->_data._string),len);
            }
            buffer_append(out,"\"",1);
        }
            break;

        case tinybuf_map:{
            if(!compact){
                add_blank(out,4 * level);
                buffer_append(out,"{\r\n",3);
            }else{
                buffer_push_inline(out,'{');
            }

            int map_size = avl_tree_num_entries(value->_data._map_array);
            if(map_size){
                for_each_context context;
                context.size = map_size;
                context.compact = compact;
                context.i = 0;
                context.out = out;
                context.level = level;
                avl_tree_for_each_node(value->_data._map_array,&context,avl_tree_for_each_node_dump_map);
            }

            if(!compact) {
                add_blank(out, 4 * level);
            }
            buffer_append(out,"}",1);
        }
            break;


        case tinybuf_array:{
            if(!compact) {
                add_blank(out, 4 * level);
                buffer_append(out, "[\r\n", 3);
            }else{
                buffer_push_inline(out,'[');
            }

            int array_size = avl_tree_num_entries(value->_data._map_array);
            if(array_size){
                for_each_context context;
                context.size = array_size;
                context.compact = compact;
                context.i = 0;
                context.out = out;
                context.level = level;
                avl_tree_for_each_node(value->_data._map_array,&context,avl_tree_for_each_node_dump_array);
            }

            if(!compact){
                add_blank(out,4 * level);
            }
            buffer_append(out,"]",1);
        }
            break;

        default:
            //不可达
            assert(0);
            break;
    }

    return 0;
}

int tinybuf_value_serialize_as_json(const tinybuf_value *value, buffer *out,int compact){
    return tinybuf_value_serialize_as_json_level(0,compact,value, out);
}


//////////////////////////////////////json解析相关//////////////////////////////////////
/**
 * 判断是否为空字符
 * @param ch 字符
 * @return 1代表是空字符
 */
static inline int is_blank(uint8_t ch){
    switch (ch){
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            return 1;
        default:
            return 0;
    }
}


/**
 * 查找相应的字符
 * @param ptr json字符串
 * @param size json字符串长度
 * @param str 查找字符集
 * @return 如果大于0，则代表找到了相关字节并返回消耗的字节数，0代表未找到，-1代表出现非法字符
 */
static inline int tinybuf_json_skip_for_simpe(const char *ptr, int size,const char *str,int show_waring){
    int i , j;
    int str_len = strlen(str);
    for(i = 0 ; i < size ; ++i){
        uint8_t ch = ((uint8_t *)ptr)[i];

        if(is_blank(ch)){
            //空白忽略之
            continue;
        }

        for(j = 0; j < str_len ; ++j){
            if(ch == str[j]){
                //消耗了这么多字节
                return i + 1;
            }
        }

        if(show_waring){
            LOGW("搜索%s时发现非法字符:%c",str,ch);
        }
        //非法字符
        return -1;
    }

    //全部都是空白,未找到
    return 0;
}

/**
 * 查找相应的字符
 * @param ptr json字符串
 * @param size json字符串长度
 * @param str 查找字符集
 * @return 如果大于0，则代表找到了相关字节并返回消耗的字节数，0代表未找到，-1代表出现非法字符
 */
static inline int tinybuf_json_skip_for(const char *ptr, int size,const char *str,int show_waring){
    const char *str_ptr = str;
    char flags[0xFF] = {0};
    while(*str_ptr){
        flags[*(str_ptr++)] = 1;
    }

    int i;
    for(i = 0 ; i < size ; ++i){
        uint8_t ch = ((uint8_t *)ptr)[i];

        if(is_blank(ch)){
            //空白忽略之
            continue;
        }

        if(flags[ch]){
            //消耗了这么多字节
            return i + 1;
        }

        if(show_waring){
            LOGW("搜索%s时发现非法字符:%c",str,ch);
        }
        //非法字符
        return -1;
    }

    //全部都是空白,未找到
    return 0;
}

static char s_flags[0xFF] = {0};
static char s_flags_ok = 0;

static inline int tinybuf_json_skip_for_complex(const char *ptr, int size,const char *str,int show_waring){
    if(!s_flags_ok){
        const char *str_ptr = str;
        while(*str_ptr){
            s_flags[*(str_ptr++)] = 1;
        }
        s_flags_ok = 1;
    }


    int i;
    for(i = 0 ; i < size ; ++i){
        uint8_t ch = ((uint8_t *)ptr)[i];

        if(is_blank(ch)){
            //空白忽略之
            continue;
        }

        if(s_flags[ch]){
            //消耗了这么多字节
            return i + 1;
        }

        if(show_waring){
            LOGW("搜索%s时发现非法字符:%c",str,ch);
        }
        //非法字符
        return -1;
    }

    //全部都是空白,未找到
    return 0;
}

/**
 * 解析Unicode字符
 * @param start json字符串
 * @param unicode unicode值
 * @return 0代表成功，-1代表失败
 */
static int decodeUnicodeEscapeSequence(const char *start,unsigned int* unicode){
    *unicode = 0;
    int index;
    for ( index = 0; index < 4; ++index) {
        char c = *(start++);
        //左移动4位
        (*unicode) <<= 4;
        if (c >= '0' && c <= '9')
            (*unicode) += c - '0';
        else if (c >= 'a' && c <= 'f')
            (*unicode) += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            (*unicode) += c - 'A' + 10;
        else{
            //出现非法字符
            LOGW("Unicode中出现非法字符:%c",c);
            return -1;
        }
    }
    return 0;
}

/**
 * Unicode转utf-8
 * @param cp unicode字符
 * @param out utf-8字符串
 */
static inline void codePointToUTF8(unsigned int cp,buffer *out) {
    // based on description from http://en.wikipedia.org/wiki/UTF-8
    if (cp <= 0x7f) {
        buffer_push_inline(out,(char)cp);
    } else if (cp <= 0x7FF) {
        buffer_push_inline(out,(char)(0xC0 | (0x1f & (cp >> 6))));
        buffer_push_inline(out,(char)(0x80 | (0x3f & cp)));
    } else if (cp <= 0xFFFF) {
        buffer_push_inline(out,(char)(0xE0 | (0xf & (cp >> 12))));
        buffer_push_inline(out,(char)(0x80 | (0x3f & (cp >> 6))));
        buffer_push_inline(out,(char)(0x80 | (0x3f & cp)));
    } else if (cp <= 0x10FFFF) {
        buffer_push_inline(out,(char)(0xF0 | (0x7 & (cp >> 18))));
        buffer_push_inline(out,(char)(0x80 | (0x3f & (cp >> 12))));
        buffer_push_inline(out,(char)(0x80 | (0x3f & (cp >> 6))));
        buffer_push_inline(out,(char)(0x80 | (0x3f & cp)));
    }
}

/**
 * 读取两个字节的Unicode
 * @param ptr json字符串
 * @param size json字符串长度
 * @param unicode Unicode值
 * @return 消耗的字节数
 */
static inline int tinybuf_load_unicode_once(const char *ptr, int size,unsigned int *unicode){
    if(size < 6){
        //字节不够
        return 0;
    }
    if(ptr[0] != '\\' || ptr[1] != 'u'){
        //非法字符
        LOGW("非法Unicode起始字符:%c%c",ptr[0],ptr[1]);
        return -1;
    }

    return -1 == decodeUnicodeEscapeSequence(ptr + 2,unicode) ? -1 : 6;
}

/**
 * 加载Unicode字符串
 * @param ptr json字符串
 * @param size json字符串长度
 * @param out 还原后的字符串保存长度
 * @return 消耗的字节数
 */
static inline int tinybuf_load_unicode(const char *ptr, int size,buffer *out){
    unsigned int unicode;
    int ret1 = tinybuf_load_unicode_once(ptr,size,&unicode);
    if(ret1 <= 0){
        //字节不够或解析失败
        return ret1;
    }
    if (unicode >= 0xD800 && unicode <= 0xDBFF) {
        // surrogate pairs
        unsigned int surrogatePair;
        int ret2 = tinybuf_load_unicode_once(ptr + ret1,size - ret1,&surrogatePair);
        if(ret2 <= 0){
            //字节不够或解析失败
            return ret2;
        }
        unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
        ret1 += ret2;
    }

    codePointToUTF8(unicode,out);
    return ret1;
}

/**
 * 加载字符串
 * @param ptr json字符串
 * @param size json字符串长度
 * @param out 转义后的字符串
 * @return 消耗的字节数
 */
static inline int tinybuf_json_load_for_string(const char *ptr, int size,buffer *out){
    int consumed = tinybuf_json_skip_for_simpe(ptr,size,"\"",1);
    if(consumed <= 0){
        //出现非法字符或未找到
        return consumed;
    }

    //是否开始转义
    int escaped_start = 0;
    int i;
    for(i = consumed; i < size ; ++i){
        uint8_t c = ((uint8_t*)ptr)[i];
        if(escaped_start){
            //开始转义了
            switch (c){
                case 'u': {
                    //上个字节是转义字符，那个是\u加4个字节的二进制
                    int offset = tinybuf_load_unicode(ptr + i - 1, size - i - 1, out);
                    if (offset <= 0) {
                        //加载Unicode失败回滚数据
                        return offset;
                    }
                    i += (offset - 2);
                }
                    break;

                case '\"':
                case '\\':
                    buffer_push_inline(out,c);
                    break;

                case 'b':
                    buffer_push_inline(out,'\b');
                    break;

                case 'f':
                    buffer_push_inline(out,'\f');
                    break;

                case 'n':
                    buffer_push_inline(out,'\n');
                    break;

                case 'r':
                    buffer_push_inline(out,'\r');
                    break;

                case 't':
                    buffer_push_inline(out,'\t');
                    break;

                default:
                    //转义字符后面出现非法字符
                    LOGW("转义字符后面出现非法字符:%c",c);
                    return -1;
            }

            //转义结束
            escaped_start = 0;
            continue;
        }

        switch (c){
            case '\"':
                //这是个引号,字符串结束了
                return i + 1;

            case '\\':
                //开始转义
                escaped_start = 1;
                break;

            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                //这些字符在string中必须转义
                LOGD("string中发现未转义的字符:%c",c);
                return -1;
            default:
                if (isControlCharacter(c) || c == 0) {
                    //这些字符必须转义
                    LOGD("string中发现未转义的字符：%c",c);
                    return -1;
                }
                buffer_push_inline(out,c);
                break;
        }
    }

    //数据不够，回滚为0
    return 0;
}

/**
 * 加载bool值
 * @param ptr json字符串
 * @param size json字符串长度
 * @param out bool值
 * @return 消耗的字节数
 */
static inline int tinybuf_json_load_for_bool(const char *ptr, int size,int *out){
    switch (ptr[0]){
        case 't':
            if(size < 4){
                //数据不够
                return 0;
            }
            if(memcmp(ptr,"true",4) == 0){
                *out = 1;
                return 4;
            }
            LOGW("判断bool true失败:%4s",ptr);
            return -1;

        case 'f':
            if(size < 5){
                //数据不够
                return 0;
            }
            if(memcmp(ptr,"false",5) == 0){
                *out = 0;
                return 5;
            }
            LOGW("判断bool false失败:%5s",ptr);
            return -1;

        default:
            return -1;
    }
}

/**
* 加载null值
* @param ptr json字符串
* @param size json字符串长度
* @return 消耗的字节数
*/
static inline int tinybuf_json_load_for_null(const char *ptr, int size){
    switch (ptr[0]){
        case 'n':
            if(size < 4){
                //数据不够
                return 0;
            }
            if(memcmp(ptr,"null",4) == 0){
                //解析成功
                return 4;
            }
            return -1;
        default:
            return -1;
    }
}

/**
 * 判断value类型
 * @param ptr json字符串
 * @param size json字符串长度
 * @param type value类型
 * @return 消耗的字节数
 */
static inline int tinybuf_json_load_for_value_type(const char *ptr, int size, tinybuf_type *type,int show_waring){
    //搜索map/array/number/string/bool/null
    int consumed = tinybuf_json_skip_for_complex(ptr,size,"{[-0123456789\"ftn",show_waring);
    if(consumed <= 0){
        //未找到或出现非法字符
        return consumed;
    }

    switch (ptr[consumed - 1]){
        case '{':
            *type = tinybuf_map;
            break;

        case '[':
            *type = tinybuf_array;
            break;

        case '\"':
            *type = tinybuf_string;
            break;

        case 't':
        case 'f':
            *type = tinybuf_bool;
            break;

        case 'n':
            *type = tinybuf_null;
            break;

        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            //double和int都共享tinybuf_int类型
            *type = tinybuf_int;
            break;

        default:
            assert(0);
            return -1;
    }

    return consumed;
}


/**
 * 加载map
 * @param ptr json字符串
 * @param size json字符串大小
 * @param map 返回值
 * @return 消耗字节数
 */
static inline int tinybuf_json_load_for_map(const char *ptr, int size, tinybuf_value *map){
    int total_consumed = 0;
    while (1) {
        //搜索string key或map结尾符
        int consumed = tinybuf_json_skip_for_simpe(ptr + total_consumed, size - total_consumed, "\"}",1);
        if (consumed <= 0) {
            //数据不够或找到非法字符
            return consumed;
        }

        //加上string key前面字节偏移量
        total_consumed += consumed - 1;

        if (ptr[total_consumed] == '}') {
            //这个是map结尾，说明map结束了
            return total_consumed + 1;
        }

        //这个是string key
        buffer *key = buffer_alloc();

        //加载string类型的key
        consumed = tinybuf_json_load_for_string(ptr + total_consumed, size - total_consumed, key);
        if (consumed <= 0) {
            //未找到key，数据不够或出现非法字符
            buffer_free(key);
            return consumed;
        }

        //加上string key的偏移量
        total_consumed += consumed;

        //找到key了，下一步找冒号
        consumed = tinybuf_json_skip_for_simpe(ptr + total_consumed, size - total_consumed, ":",1);
        if (consumed <= 0) {
            //未找到冒号
            buffer_free(key);
            return consumed;
        }

        //加上冒号偏移量
        total_consumed += consumed;

        tinybuf_value *value = tinybuf_value_alloc();
        consumed = tinybuf_value_deserialize_from_json(ptr + total_consumed, size - total_consumed, value);
        if (consumed <= 0) {
            //数据不够或出现非法字符
            buffer_free(key);
            tinybuf_value_free(value);
            return consumed;
        }

        //加上value的偏移量
        total_consumed += consumed;

        //保存key-value键值对
        tinybuf_value_map_set2(map, key, value);

        //搜索逗号或结尾符号
        consumed = tinybuf_json_skip_for_simpe(ptr + total_consumed, size - total_consumed, ",}",1);
        if (consumed <= 0) {
            //未找到了逗号和map结尾符号
            return consumed;
        }

        //加上偏移量
        total_consumed += consumed;
        if(ptr[total_consumed - 1] == '}'){
            //找到map结尾
            return total_consumed;
        }

        //还有下一个对象
    }
}

static inline int tinybuf_value_deserialize_from_json_l(const char *ptr, int size, tinybuf_value *out,int show_waring);

/**
 * 从json中加载array
 * @param ptr json字符串
 * @param size json字符串长度
 * @param array 数组
 * @return 消耗字节数
 */
static inline int tinybuf_json_load_for_array(const char *ptr, int size, tinybuf_value *array){
    int total_consumed = 0;
    while (1) {
        //搜索子对象
        tinybuf_value *child = tinybuf_value_alloc();
        int consumed = tinybuf_value_deserialize_from_json_l(ptr + total_consumed, size - total_consumed, child,0);
        if (consumed <= 0) {
            //未找到子对象
            if (consumed == -1) {
                //非法字符，那么尝试查找array的末尾
                consumed = tinybuf_json_skip_for_simpe(ptr + total_consumed, size - total_consumed, "]",1);
                if (consumed <= 0) {
                    //数据不够或找到非法字符
                    tinybuf_value_free(child);
                    return consumed;
                }
                //找到arrary的末尾
                tinybuf_value_free(child);
                return total_consumed + consumed;
            }
            //数据不够
            tinybuf_value_free(child);
            return consumed;
        }

        //加上子对象的偏移量
        total_consumed += consumed;
        //保存子对象
        tinybuf_value_array_append(array, child);

        //查找逗号或array结尾符号
        consumed = tinybuf_json_skip_for_simpe(ptr + total_consumed, size - total_consumed, ",]",1);
        if (consumed <= 0) {
            //未找到了逗号和array结尾符号
            return consumed;
        }

        //加上偏移量
        total_consumed += consumed;
        if(ptr[total_consumed - 1] == ']'){
            //找到array结尾
            return total_consumed;
        }
        //还有下一个对象
    }
}

int scanf_int(const char *str,int str_len,int64_t *out){
    int is_negtive = 0;
    *out = 0;
    for(int i = 0; i < str_len; ++i){
        if(i == 0 && str[i] == '-'){
            is_negtive = 1;
            continue;
        }
        char c = str[i];
        if (c < '0' || c > '9'){
            return -1;
        }
        *out = 10 * (*out) + (c - '0');
    }
    if(is_negtive){
        *out = -*out;
    }
    return 0;
}

/**
 * 加载double或int型数据
 * @param ptr json字符串
 * @param size json字符串长度
 * @param number 返回的数据对象
 * @return 消耗字节数
 */
static inline int tinybuf_json_load_for_number(const char *ptr, int size, tinybuf_value *number){
    char *p = (char*)ptr;
    const char *end = ptr + size;
    int is_double = 0;

    char c = *(++p); // stopgap for already consumed character
    // integral part
    while (c >= '0' && c <= '9'){
        c =   p < end ? *(++p) : 0;
    }

    // fractional part
    if (c == '.') {
        is_double = 1;
        c = p < end ? *(++p) : 0;
        while (c >= '0' && c <= '9'){
            c = p < end ? *(++p) : 0;
        }
    }
    // exponential part
    if (c == 'e' || c == 'E') {
        is_double = 1;
        c = p < end ? *(++p) : 0;
        if (c == '+' || c == '-')
            c = p < end ? *(++p) : 0;
        while (c >= '0' && c <= '9'){
            c = p < end ? *(++p) : 0;
        }
    }


    if(!is_double){
        //这是int
        int64_t int_val;
        if(-1 == scanf_int(ptr,p - ptr,&int_val)){
            return -1;
        }
        tinybuf_value_init_int(number, int_val);
        return p - ptr;
    }

    //这是double
    if(size >  p - ptr){
        //末尾还有字节
        char tmp = *p;
        *p = '\0';
        tinybuf_value_init_double(number, atof(ptr));
        *p = tmp;
    }else{
        //确保最后一个字节为'\0'
        buffer *buf = buffer_alloc();
        buffer_assign(buf,ptr,p - ptr);
        tinybuf_value_init_double(number, atof(buffer_get_data_inline(buf)));
        buffer_free(buf);
    }
    return p - ptr;
}


/**
 * 加载value部分
 * @param ptr json字符串
 * @param size json字符串长度
 * @param out vlue值输出
 * @return 消耗的字节数
 */
int tinybuf_value_deserialize_from_json(const char *ptr, int size, tinybuf_value *out){
    return tinybuf_value_deserialize_from_json_l(ptr,size,out,1);
}

static int tinybuf_value_deserialize_from_json_l(const char *ptr, int size, tinybuf_value *out,int show_waring){
    assert(out->_type == tinybuf_null);
    tinybuf_type type;
    int total_consumed = tinybuf_json_load_for_value_type(ptr,size,&type,show_waring);
    if(total_consumed <= 0){
        //数据不够或解析失败
        return total_consumed;
    }

    switch (type){
        case tinybuf_map: {
            int consumed = tinybuf_json_load_for_map(ptr + total_consumed,size - total_consumed,out);
            if(consumed <= 0){
                //数据不够或出现非法字符
                tinybuf_value_clear(out);
                return consumed;
            }
            //设置为map类型，防止空map时未设置类型
            out->_type = tinybuf_map;
            return total_consumed + consumed;
        }

        case tinybuf_array: {
            int consumed = tinybuf_json_load_for_array(ptr + total_consumed, size - total_consumed, out);
            if (consumed <= 0) {
                //数据不够或出现非法字符
                tinybuf_value_clear(out);
                return consumed;
            }
            //设置为array类型，防止空array时未设置类型
            out->_type = tinybuf_array;
            return total_consumed + consumed;
        }

        case tinybuf_string: {
            //string必须从双引号开始
            total_consumed -= 1;
            buffer *str = buffer_alloc();
            int consumed = tinybuf_json_load_for_string(ptr + total_consumed, size - total_consumed, str);
            if (consumed <= 0) {
                //数据不够或出现非法字符
                buffer_free(str);
                return consumed;
            }

            tinybuf_value_init_string2(out,str);
            return total_consumed + consumed;
        }

        case tinybuf_bool: {
            //bool必须从true或false第一个字节开始
            total_consumed -= 1;
            int flag;
            int consumed = tinybuf_json_load_for_bool(ptr + total_consumed, size - total_consumed, &flag);
            if (consumed <= 0) {
                //数据不够或出现非法字符
                return consumed;
            }
            //设置value
            tinybuf_value_init_bool(out, flag);
            return total_consumed + consumed;

        }

        case tinybuf_int: {
            //number必须从第一个字节开始
            total_consumed -= 1;
            int consumed = tinybuf_json_load_for_number(ptr + total_consumed, size - total_consumed, out);
            if (consumed <= 0) {
                //数据不够或出现非法字符
                return consumed;
            }
            return total_consumed + consumed;
        }

        case tinybuf_null: {
            //null必须从第一个字节开始
            total_consumed -= 1;
            int consumed = tinybuf_json_load_for_null(ptr + total_consumed, size - total_consumed);
            if (consumed <= 0) {
                //数据不够或出现非法字符
                return consumed;
            }

            //设置value为null
            tinybuf_value_clear(out);
            return total_consumed + consumed;
        }

        default:
            assert(0);
            return -1;
    }
}

