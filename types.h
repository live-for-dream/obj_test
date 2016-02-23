#ifndef _TYPES_INCLUDED
#define _TYPES_INCLUDED
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char uchar;
typedef unsigned int  uint;


typedef struct string_s {
    uchar           *str;
    uint             len;
} string_t;

typedef struct hash_s {
    size_t      size;
    elements    *elt;
} hash_t;

#define init_string(str)\
    str.str = NULL;\
    str.len = 0
#define NULL_STRING(name)\
    string_t name = {.str = NULL, .len = 0}

int string_to_hex(string_t str, string_t hex);
int hex_to_string(string_t str, string_t hex); 



#define OK 0
#define ERR -1
#define ERR_NULL -2
#define ERR_NOMEM -3

#endif
