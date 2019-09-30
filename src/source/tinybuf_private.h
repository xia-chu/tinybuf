#ifndef TINYBUF_PRIVATE_H
#define TINYBUF_PRIVATE_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "avl-tree.h"
#include "tinybuf.h"
#include "tinybuf_memory.h"
#include "tinybuf_log.h"
#include "tinybuf_buffer_private.h"

typedef struct tinybuf_value {
    union {
        int64_t _int;
        int _bool;
        double _double;
        buffer *_string;
        AVLTree *_map_array;
    } _data;
    tinybuf_type _type;
}tinybuf_value;

#endif//TINYBUF_PRIVATE_H



