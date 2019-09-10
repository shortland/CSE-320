#include "debug.h"
#include "transplant.h"
#include "stdio.h"
#include "unistd.h"
#include "stdint.h"

int write_magic_bytes() {
    if (putchar(MAGIC0) == EOF) return -1;
    if (putchar(MAGIC1) == EOF) return -1;
    if (putchar(MAGIC2) == EOF) return -1;
    return 0;
}

int write_type(char type) {
    if (putchar(type) == EOF) return -1;
    return 0;
}

void write_int32(uint32_t bignum) {
    int byte_a = *(((unsigned char *)(&bignum)) + 3);
    int byte_b = *(((unsigned char *)(&bignum)) + 2);
    int byte_c = *(((unsigned char *)(&bignum)) + 1);
    int byte_d = *(((unsigned char *)(&bignum)) + 0);
    putchar(byte_a);
    putchar(byte_b);
    putchar(byte_c);
    putchar(byte_d);
}

int write_depth(uint32_t depth) {
    write_int32(depth);
    return 0;
}

void write_int64(uint64_t bignum) {
    int byte_a = *(((unsigned char *)(&bignum)) + 7);
    int byte_b = *(((unsigned char *)(&bignum)) + 6);
    int byte_c = *(((unsigned char *)(&bignum)) + 5);
    int byte_d = *(((unsigned char *)(&bignum)) + 4);
    int byte_e = *(((unsigned char *)(&bignum)) + 3);
    int byte_f = *(((unsigned char *)(&bignum)) + 2);
    int byte_g = *(((unsigned char *)(&bignum)) + 1);
    int byte_h = *(((unsigned char *)(&bignum)) + 0);
    putchar(byte_a);
    putchar(byte_b);
    putchar(byte_c);
    putchar(byte_d);
    putchar(byte_e);
    putchar(byte_f);
    putchar(byte_g);
    putchar(byte_h);
}

int write_size(uint64_t size) {
    write_int64(size);
    return 0;
}

int write_record_start() {
    if (write_magic_bytes() == -1) return -1;
    if (write_type(START_OF_TRANSMISSION) == -1) return -1;
    if (write_depth((uint32_t) 0) == -1) return -1;
    if (write_size((uint64_t) HEADER_SIZE) == -1) return -1;
    return 0;
}

int write_record_end() {
    if (write_magic_bytes() == -1) return -1;
    if (write_type(END_OF_TRANSMISSION) == -1) return -1;
    if (write_depth((uint32_t) 0) == -1) return -1;
    if (write_size((uint64_t) HEADER_SIZE) == -1) return -1;
    return 0;
}

int write_record_dir_start() {
    // TODO
    return 0;
}

int write_record_dir_end() {
    // TODO
    return 0;
}

int write_record_dir_entry() {
    // TODO
    return 0;
}

int write_record_file_data() {
    // TODO
    return 0;
}
