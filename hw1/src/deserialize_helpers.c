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
            error("read char was not expected byte");
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

int read_directory_start(int depth) {
    debug("attempting to match magic bytes");
    if (match_magic_bytes() == -1) {
        return -1;
    }

    debug("attempting to match type byte");
    if (match_type(2) == -1) {
        return -1;
    }

    debug("attempting to match depth bytes");
    if (match_depth(depth) == -1) {
        return -1;
    }

    debug("attempting to match size bytes");
    if (match_size(16) == -1) {
        return -1;
    }

    debug("successfully read a START_OF_DIRECTORY record");

    return 0;
}

int read_directory_end(int depth) {
    debug("attempting to match magic bytes");
    if (match_magic_bytes() == -1) {
        return -1;
    }

    debug("attempting to match type byte");
    if (match_type(3) == -1) {
        return -1;
    }

    debug("attempting to match depth bytes");
    if (match_depth(depth) == -1) {
        return -1;
    }

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
struct Metadata read_dir_entry_data(int depth) {
    debug("attempting to match magic bytes");
    if (match_magic_bytes() == -1) {
        return (Metadata) {.error = -1};
    }

    debug("attempting to match type byte");
    if (match_type(4) == -1) {
        return (Metadata) {.error = -1};
    }

    debug("attempting to match depth bytes");
    if (match_depth(depth) == -1) {
        return (Metadata) {.error = -1};
    }

    debug("attempting to get size of record");
    uint64_t recordSize;
    if ((recordSize = read_record_size()) == -1) {
        return (Metadata) {.error = -1};
    }
    // remove header //and metadata bytes
    recordSize -= (16);

    // also holds info about the type (dir or file)
    debug("attempting to get permissions bytes");
    mode_t permissions;
    if ((permissions = read_permissions()) == -1) {
        return (Metadata) {.error = -1};
    }
    debug("permissions are %d", permissions);
    // remove the read permission bytes from size
    //recordSize -= 4;

    // should be done outsied this function
    // debug("attempting to get file size");
    // off_t fileSize;
    // if ((fileSize = read_meta_file_size()) == -1) {
    //     return -1;
    // }

    // debug("attempting to get dir/file name");

    return (Metadata) {
        .error = 0,
        .permissions = permissions,
        .size = recordSize
    };

    // debug("attempting to read record type");
    // int recordType;
    // if ((recordType = read_byte()) == -1) {
    //     return -1;
    // }

    // return recordType;

    // GET THE TYPE, make sure the type it got is a "directory entry"
    // then, return whether it is a file or a directory type
    // (this is parsed from the metadata of a directory entry record)
}
