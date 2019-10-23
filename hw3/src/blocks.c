#include <stdlib.h>

#include "blocks.h"

size_t blocks_minimum_size(size_t size) {
    int amt = 16 - (size % 16);

    return size + amt;
}