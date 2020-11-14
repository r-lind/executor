#pragma once

#include <stdint.h>

#define NOTKEY 0x89

typedef struct key_table_data
{
    uint8_t high_byte;
    int min;
    int size;
    uint8_t *data;
} key_table_t;

extern key_table_t key_tables[];