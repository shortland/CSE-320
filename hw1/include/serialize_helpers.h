#ifndef SERIALIZE_HELPERS_H
#define SERIALIZE_HELPERS_H

int write_record_start();

int write_record_end();

int stdout_header_file_data();

int write_magic_bytes();

int write_type(char type);

int write_depth(uint32_t depth);

void write_recursive_int(uint32_t bignum);

int write_record_dir_start();

int write_record_dir_end();

int write_record_dir_entry();

int write_record_file_data();

#endif