#include <stdio.h>
#include <stdint.h>

#include "debug.h"
#include "const.h"
#include "string_helpers.h"
#include "deserialize_helpers.h"

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
