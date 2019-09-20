#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"
#include "string_helpers.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv) {
    if (validargs(argc, argv) == -1) {
        debug("validargs returned error");
        USAGE(*argv, EXIT_FAILURE);
    }

    debug("Options: 0x%x", global_options);
    if (global_options & 1) {
        debug("show usage success");
        USAGE(*argv, EXIT_SUCCESS);
    }

    /**
     * For both -s and -d
     */
    if (string_length(name_buf) == 0) {
        debug("using cwd, no name_buf available");

        if (path_init("./") == -1) {
            error("path_init() error");
            return EXIT_FAILURE;
        }
    } else {
        debug("using name_buf set to: %s", name_buf);

        if (path_init(name_buf) == -1) {
            error("path_init() error");
            return EXIT_FAILURE;
        }
    }

    /**
     * When -s is flagged
     */
    if (global_options & (1 << 1)) {
        debug("-s, serialization mode");

        if (serialize() == -1) {
            error("serialize failed");
            return EXIT_FAILURE;
        } else {
            debug("serialized successfully");
            fflush(stdout);
        }
    }

    /**
     * When -d is flagged
     */
    if (global_options & (1 << 2)) {
        debug("-d, deserialization mode");

        if (deserialize() == -1) {
            error("deserialize failed");
            return EXIT_FAILURE;
        } else {
            debug("deserialized successfully");
            fflush(stdout);
        }
    }

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
