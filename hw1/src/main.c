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

int main(int argc, char **argv)
{
    int ret;
    if (validargs(argc, argv) == -1) {
        debug("validargs returned error");
        USAGE(*argv, EXIT_FAILURE);
    }

    debug("Options: 0x%x", global_options);
    if (global_options & 1) {
        debug("show usage success");
        USAGE(*argv, EXIT_SUCCESS);
    }

    if (global_options & (1 << 1)) {
        debug("-s, serialization mode");
    }

    if (global_options & (1 << 2)) {
        debug("-d, deserialization mode");
    }

    if (string_length(name_buf) > 0) {
        debug("name_buf set to: %s", name_buf);
    }

    /* begin serialization/deserialization */
    //return begin_transplant();

    // test path_init()
    // is this correct?
    if (string_length(name_buf) == 0) {
        if (path_init("./") == -1) {
            error("path_init() error");
            return EXIT_FAILURE;
        }
    } else {
        path_init(name_buf);
    }


    // test path_push();
    debug("current path_buf: %s", path_buf);
    if (path_push("someDirectory") == -1) {
        error("path_push() error");
        return EXIT_FAILURE;
    }
    debug("after appending path_buf: %s, and new path_length is %d", path_buf, path_length);

    debug("old path_buf is: %s", path_buf);
    if (path_pop() == -1) {
        error("path_pop() error");
        return EXIT_FAILURE;
    }
    debug("new path_buf is: %s", path_buf);
    if (path_pop() == -1) {
        error("path_pop() error");
        return EXIT_FAILURE;
    }
    debug("new path_buf is: %s", path_buf);

    serialize();

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
