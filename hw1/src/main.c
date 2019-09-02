#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

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
    if (validargs(argc, argv)) {
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

    // if (global_options & 4) {
    //     debug("-d, deserialization mode");
    // }

    // if (global_options & 2) {
    //     debug("-s, serialization mode");
    // }

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
