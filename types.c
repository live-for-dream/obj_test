#include "types.h"

int string_to_hex(string_t str, string_t hex) {
    int i;
    
    if (!str || !str->len || !hex) {
        return ERR_NULL;
    }

    hex->str = malloc(sizeof(uchar) * str->len * 2 + 1);
    if (!hex->str) {
        return ERR_NOMEM;
    }
    hex->len = str->len * 2;
    
    for (i = 0; i < str->len; i++) {
        sprintf((char *)(hex->str + 2 * i), "%02X", (char)str->str[i]);    
    }
    
    hex->str[hex->len] = '\0';
    return OK;
}
 
int hex_to_string(string_t str, string_t hex) {
    int i;
    uchar tmp;

    if (!str || !hex || !hex->len) {
        return ERR_NULL;
    }

    str->str = malloc(sizeof(uchar) * hex->len / 2 + 1);
    if (!str->str) {
        return ERR_NOMEM;
    }
    
    str->len = str->len / 2;
    for (i = 0; i < str->len; i++) {
        tmp = 0x00;
        tmp = hex->str[2 * i];
        tmp <<= 4;
        tmp |= hex->str[2 * i + 1] & 0x0f;
        str->str[i] = tmp;
    }

    str->str[i] = '\0';
    return OK;
}

