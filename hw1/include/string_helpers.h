#ifndef STRING_HELPERS_H
#define STRING_HELPERS_H

int string_length(char *string);

void copy_string_and_null(char *source, char *dest);

void copy_string_no_null(char *source, char *dest);

int string_contains_char(char *string, char needle);

void append_string_to_existing(char *existing, char *toAppend);

void remove_suffix_at_char(char *source, char a);

int position_of_char_from_suffix(char *source, char a);

int string_equals(char *first, char *second);

int read_stdin_into_name(char *dest, int startAtPos, int length);

#endif
