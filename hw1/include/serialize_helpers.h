#ifndef SERIALIZE_HELPERS_H
#define SERIALIZE_HELPERS_H

#include "debug.h"
#include "transplant.h"
#include "stdio.h"
#include "unistd.h"
#include "stdint.h"
#include "string_helpers.h"
#include <sys/stat.h>

int write_magic_bytes();

int write_type(char type);

int write_num_bytes(int numBytes, uint64_t bignum);

int write_depth(uint32_t depth);

int write_size(uint64_t size);

int write_record_start();

int write_record_end();

int write_record_dir_entry(mode_t mode, off_t sizeInfo, uint32_t depth, char *name);

int write_record_dir_start();

int write_record_dir_end();

int write_record_file_data();

#endif