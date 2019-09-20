#include <stdio.h>
#include <stdint.h>

#include "debug.h"
#include "const.h"
#include "string_helpers.h"
#include "deserialize_helpers.h"

int read_byte() {
    int readChar;

    if ((readChar = getchar()) == EOF) {
        error("read char was EOF");

        return -1;
    }

    return readChar;
}

uint64_t read_bytes(int amt) {
    if (amt > 8) {
        error("match_raw_bytes() cannot handle more than 8 bytes of matching; \
            it can be improved to do so though.");

        return -1;
    }

    uint64_t size = 0;
    uint64_t byte;

    for (int i = 0; i < amt; ++i) {
        if ((byte = read_byte()) == -1) {
            error("unable to read byte");

            return -1;
        }

        size |= ( byte << ((8 * amt) - ((i + 1) * 8)) );
    }

    return size;
}

off_t read_meta_file_size() {
    return read_bytes(8);
}

mode_t read_permissions() {
    return read_bytes(4);
}

uint64_t read_record_size() {
    return read_bytes(8);
}

int read_byte_expected(int expected) {
    int readChar;

    if ((readChar = getchar()) == EOF) {
        error("read char was EOF");

        return -1;
    } else if (readChar == expected) {
        debug("read char matches expected");

        return 0;
    } else {
        error("read char does not match expected");

        return -1;
    }
}

int match_magic_bytes() {
    if (read_byte_expected(MAGIC0) == -1) {
        return -1;
    }

    if (read_byte_expected(MAGIC1) == -1) {
        return -1;
    }

    if (read_byte_expected(MAGIC2) == -1) {
        return -1;
    }

    return 0;
}

int match_type(char type) {
    if (read_byte_expected(type) == -1) {
        return -1;
    }

    return 0;
}

/**
 * Matches up to 8 bytes
 */
int match_raw_bytes(uint64_t bytes, int amount) {
    if (amount > 8) {
        error("match_raw_bytes() cannot handle more than 8 bytes of matching; \
            it can be improved to do so though.");

        return -1;
    }

    int rawByte;
    int readChar;

    for (int i = 0; i < amount; i++) {
        rawByte = *(((unsigned char *)(&bytes)) + (amount - 1 - i));

        if ((readChar = getchar()) == EOF) {
            error("read char was EOF");

            return -1;
        } else if (readChar == rawByte) {
            debug("readchar was the expected byte!");
        } else {
            error("read char was not expected byte; expected: %d; actual: %d", rawByte, readChar);

            return -1;
        }
    }

    return 0;
}

int match_depth(uint32_t depth) {
    if (match_raw_bytes(depth, 4) == -1) {
        error("error matching depth bytes");

        return -1;
    }

    return 0;
}

int match_size(uint64_t size) {
    if (match_raw_bytes(size, 8) == -1) {
        error("error matching size bytes");

        return -1;
    }

    return 0;
}

int validate_record_return_type(int depth) {
    debug("attempting to match magic bytes");

    if (match_magic_bytes() == -1) {
        return -1;
    }

    debug("attempting to read type byte");

    int type;

    if ((type = read_byte()) == -1) {
        return -1;
    }

    debug("got type of %d", type);

    debug("attempting to match depth bytes");

    if (match_depth(depth) == -1) {
        return -1;
    }

    return type;
}

int read_record_start() {
    debug("attempting to match magic bytes");

    if (match_magic_bytes() == -1) {
        return -1;
    }

    debug("attempting to match type byte");

    if (match_type(0) == -1) {
        return -1;
    }

    debug("attempting to match depth bytes");

    if (match_depth(0) == -1) {
        return -1;
    }

    debug("attempting to match size bytes");

    if (match_size(16) == -1) {
        return -1;
    }

    debug("successfully read a START_OF_TRANSMISSION record");

    return 0;
}

int read_record_end() {
    debug("attempting to match magic bytes");

    if (match_magic_bytes() == -1) {
        return -1;
    }

    debug("attempting to match type byte");

    if (match_type(1) == -1) {
        return -1;
    }

    debug("attempting to match depth bytes");

    if (match_depth(0) == -1) {
        return -1;
    }

    debug("attempting to match size bytes");

    if (match_size(16) == -1) {
        return -1;
    }

    debug("successfully read a END_OF_TRANSMISSION record");

    return 0;
}

int read_directory_start() {
    debug("attempting to match size bytes");

    if (match_size(16) == -1) {
        return -1;
    }

    debug("successfully read a START_OF_DIRECTORY record");

    return 0;
}

int read_directory_end() {
    debug("attempting to match size bytes");

    if (match_size(16) == -1) {
        return -1;
    }

    debug("successfully read a END_OF_DIRECTORY record");

    return 0;
}

/**
 * Reads a DIRECTORY_ENTRY record,
 * And returns the parsed out the metadata of the directory entry
 */


// this function should return mode_t (permissions and type)
// and also should return the: recordSize (so that dir name and the rest can be parsed out)
struct Metadata read_dir_entry_data() {
    debug("attempting to get size of record");

    uint64_t recordSize;

    if ((recordSize = read_record_size()) == -1) {
        return (Metadata) {.error = -1};
    }
    // remove header and metadata bytes
    recordSize -= (16 + 12);

    // also holds info about the type (dir or file)
    debug("attempting to get permissions bytes");

    mode_t permissions;

    if ((permissions = read_permissions()) == -1) {
        return (Metadata) {.error = -1};
    }

    debug("permissions are %d", permissions);

    debug("attempting to get filesize bytes");

    off_t fileSize;

    if ((fileSize = read_meta_file_size()) == -1) {
        return (Metadata) {.error = -1};
    }

    debug("filesize bytes are %ld", fileSize);

    return (Metadata) {
        .error = 0,
        .permissions = permissions,
        .size = recordSize,
        .fileSize = fileSize
    };
}

int read_file_data_make_file(int depth, char *path) {
    debug("attempting to match magic bytes");

    if (match_magic_bytes() == -1) {
        return -1;
    }

    debug("attempting to match type byte");

    if (match_type(5) == -1) {
        return -1;
    }

    debug("attempting to match depth bytes");

    if (match_depth(depth) == -1) {
        return -1;
    }

    debug("attempting to get size of record");

    uint64_t fileSize;

    if ((fileSize = read_record_size()) == -1) {
        return -1;
    }

    fileSize -= 16;
    debug("filesize is %ld", fileSize);

    FILE *fp = fopen(path, "w");

    if (fp == NULL) {
        error("couldn't open file");

        return -1;
    } else {
        int readChar;

        debug("attempting to read stdin and write bytes to file");

        for (int i = 0; i < fileSize; ++i) {
            if ((readChar = read_byte()) == -1) {
                error("error reading a byte for filedata");

                return -1;
            }

            debug("writing byte %d to file", readChar);

            if (fputc(readChar, fp) == EOF) {
                error("unable to write char to file, got EOF");

                return -1;
            }
        }

        if (fclose(fp) == -1) {
            error("unable to close file pointer");

            return -1;
        }
    }

    debug("successfully wrote bytes to file");

    return 0;
}

/**
 * Just create the directory
 */
int create_dir(char *path) {
    if (mkdir(path, 0700) == -1) {
        error("unable to create directory");

        return -1;
    }

    debug("successfully created directory");

    return 0;
}
