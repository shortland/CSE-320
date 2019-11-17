#ifndef COMMANDS_H
#define COMMANDS_H

extern char COMMANDS_LIST[12][10];

typedef struct COMMANDS_METADATA {
    int valid;
    char *command; // should be free'd
    int valid_args;
    char *whole_args; // should be free'd ? (is this already the input_line from read_line in main while)
} COMMANDS_METADATA;

/**
 * Takes in raw string, and returns whether the string is valid,
 * what the command is, and the string itself again in a struct.
 * @returns COMMAND_METADATA
 * ->valid = 0 if valid command, -1 otherwise.
 */
COMMANDS_METADATA commands_parse(char *string);

/**
 * Checks whether the supplied string exists in COMMANDS_LIST[]
 * @returns 0 if it does, otherwise -1
 */
int commands_is_command(char *command);

/**
 * If the string starts & ends with single quotes then returns string without quotes.
 * If the string does not have single quotes, it must be a single word without spaces. Otherwise return -1
 * @returns -1 on error, 0 on standard word, 1 on multi-word arg with quotes
 */
int commands_is_args(char *string);

/**
 * Expects quotes to be there.
 * Must be free'd.
 * @returns string without single quotes at the beginning and end of the string.
 */
char *remove_quotes(char *command);

/**
 *
 */

#endif // COMMANDS_H
