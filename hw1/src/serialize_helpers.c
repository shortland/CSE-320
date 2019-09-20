#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "debug.h"
#include "transplant.h"
#include "string_helpers.h"

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

int write_num_bytes(int numBytes, uint64_t bignum) {
    int byte;
    for (int i = 0; i < numBytes; i++) {
        byte = *(((unsigned char *)(&bignum)) + (numBytes - 1 - i));
        if (putchar(byte) == EOF) return -1;
    }
    return 0;
}

int write_depth(uint32_t depth) {
    return write_num_bytes(4, depth);
}

int write_size(uint64_t size) {
    return write_num_bytes(8, size);
}

/**
 * START_OF_TRANSMISSION = 0
 *
 */
int write_record_start() {
    if (write_magic_bytes() == -1) return -1;
    if (write_type(START_OF_TRANSMISSION) == -1) return -1;
    if (write_depth((uint32_t) 0) == -1) return -1;
    if (write_size((uint64_t) HEADER_SIZE) == -1) return -1;
    return 0;
}

/**
 * END_OF_TRANSMISSION = 1
 *
 */
int write_record_end() {
    if (write_magic_bytes() == -1) return -1;
    if (write_type(END_OF_TRANSMISSION) == -1) return -1;
    if (write_depth((uint32_t) 0) == -1) return -1;
    if (write_size((uint64_t) HEADER_SIZE) == -1) return -1;
    return 0;
}

/**
 * DIRECTORY_ENTRY = 4
 *
 */
int write_record_dir_entry(uint32_t mode, off_t sizeInfo, uint32_t depth, char *name) {
    // Entry name ? bytes
    int amt = string_length(name);

    // Header; 16 bytes
    amt += (16 + 12);
    if (write_magic_bytes() == -1) return -1;
    if (write_type(DIRECTORY_ENTRY) == -1) return -1;
    if (write_depth((uint32_t) depth) == -1) return -1;
    if (write_size((uint64_t) amt) == -1) return -1;

    // Metadata; 12 bytes
    if (write_num_bytes(4, mode) == -1) return -1;
    if (write_num_bytes(8, sizeInfo) == -1) return -1;

    for (int i = 0; i < string_length(name); i++) {
        write_num_bytes(1, *(name + i));
    }

    return 0;
}

/**
 * START_OF_DIRECTORY
 */
int write_record_dir_start(uint32_t depth) {
    if (write_magic_bytes() == -1) return -1;
    if (write_type(START_OF_DIRECTORY) == -1) return -1;
    if (write_depth((uint32_t) depth) == -1) return -1;
    if (write_size((uint64_t) HEADER_SIZE) == -1) return -1;
    return 0;
}

/**
 * END_OF_DIRECTORY
 */
int write_record_dir_end(uint32_t depth) {
    if (write_magic_bytes() == -1) return -1;
    if (write_type(END_OF_DIRECTORY) == -1) return -1;
    if (write_depth((uint32_t) depth) == -1) return -1;
    if (write_size((uint64_t) HEADER_SIZE) == -1) return -1;
    return 0;
}

/**
 * FILE_DATA
 * [Header Only]
 * @return number of bytes written
 */
int write_record_file_data(uint32_t depth, char *filepath, off_t size) {
    if (write_magic_bytes() == -1) return -1;
    if (write_type(FILE_DATA) == -1) return -1;
    if (write_depth((uint32_t) depth) == -1) return -1;
    int totalSize = size + HEADER_SIZE;
    if (write_size((uint64_t) totalSize) == -1) return -1; // size

    FILE *f = fopen(filepath, "r");
    if (f == NULL) {
        error("error opening file to write");
        return -1;
    }

    int i;
    int getChar;
    for (i = 0; i < size; i++) {
        getChar = fgetc(f);

        if (feof(f)) {
            error("fgetc reached EOF");
            fclose(f);
            return -1;
        }

        if (putchar(getChar) == EOF) {
            error("putchar reached EOF");
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    return i;
}
