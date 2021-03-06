#include <stdio.h>

#include "debug.h"

int string_length(char *string) {
    int c = 0;

    while (*(string + c) != '\0') {
        c++;
    }

    return c;
}

void copy_string_and_null(char *source, char *dest) {
    int c = 0;

    while (*(source + c) != '\0') {
        *(dest + c) = *(source + c);
        c++;
    }

    // copy over the null pointer
    *(dest + c) = *(source + c);

    debug("copied result %s", dest);
}

void copy_string_no_null(char *source, char *dest) {
    int c = 0;

    while (*(source + c) != '\0') {
        *(dest + c) = *(source + c);
        c++;
    }

    debug("copied result %s", dest);
}

int string_contains_char(char *string, char needle) {
    int c = 0;

    while (*(string + c) != '\0') {
        if (*(string + c) == needle) {
            debug("string does contain needle");
            return 0;
        }

        c++;
    }

    debug("string: '%s' does not contain needle: '%c'", string, needle);

    return -1;
}

void append_string_to_existing(char *existing, char *toAppend) {
    int c = 0;

    while (*(existing + c) != '\0') {
        c++;
    }

    int d = 0;

    while (*(toAppend + d) != '\0') {
        *(existing + c + d) = *(toAppend + d);
        d++;
    }

    *(existing + c + d) = '\0';
}

void remove_suffix_at_char(char *source, char a) {
    int c = 1;
    int sourceLength = string_length(source);

    while (sourceLength - c >= 0) {
        if (*(source + (sourceLength - c)) == a) {
            *(source + (sourceLength - c)) = '\0';
            break;
        }

        c++;
    }
}

int position_of_char_from_suffix(char *source, char a) {
    int c = 1;
    int sourceLength = string_length(source);

    while (sourceLength - c >= 0) {
        if (*(source + (sourceLength - c)) == a) {
            return sourceLength - c;
        }

        c++;
    }

    return -1;
}

int string_equals(char *first, char *second) {
    if (string_length(first) != string_length(second)) {
        return -1;
    }

    for (int i = 0; i < string_length(first); i++) {
        if (*(first + i) != *(second + i)) {
            return -1;
        }
    }

    return 0;
}

int read_stdin_into_name(char *dest, int startAtPos, int length) {
    int readChar;

    int i;
    for (i = 0; i < length; ++i) {
        if ((readChar = getchar()) == EOF) {
            error("read char was EOF");
            return -1;
        }

        *(dest + (startAtPos + i)) = readChar;
    }

    *(dest + (startAtPos + i)) = '\0';

    return 0;
}