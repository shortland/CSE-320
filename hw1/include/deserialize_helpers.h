#ifndef DESERIALIZE_HELPERS_H
#define DESERIALIZE_HELPERS_H

typedef struct Metadata {
    int error;
    uint64_t size;
    mode_t permissions;
    off_t fileSize;
} Metadata;

int read_byte();

int read_byte_expected(int expected);

int match_magic_bytes();

int match_type(char type);

int match_raw_bytes(uint64_t bytes, int amount);

int match_depth(uint32_t depth);

int match_size(uint64_t size);

int read_record_start();

int read_record_end();

int read_directory_start();

int read_directory_end();

struct Metadata read_dir_entry_data();

int read_file_data_make_file(int depth, char *path);

int read_dir_name_create_dir(char *path);

int validate_record_return_type(int depth);

#endif
