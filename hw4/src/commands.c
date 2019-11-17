#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "commands.h"

char COMMANDS_LIST[12][10] = {
    "help",
    "quit",
    "enable",
    "disable",
    "spool",
    "pause",
    "resume",
    "cancel",
    "expunge",
    "status",
    "jobs",
    "",
};

// expects quotes to definitely be there.
char *remove_quotes(char *command) {
    int length = strlen(command);
    char *cleaned = malloc(sizeof(char) * (length - 1));

    int i;
    for (i = 0; i < length - 2; i++) {
        cleaned[i] = command[i + 1];
    }

    cleaned[i] = '\0';

    return cleaned;
}

int commands_is_command(char *command) {
    int i = 0;

    while ( strcmp(COMMANDS_LIST[i], "") != 0 ) {
        if ( strcmp(COMMANDS_LIST[i], command) == 0 ) {
            return 0;
        }

        i++;
    }

    return -1;
}

int commands_is_args(char *string) {
    if (string == NULL) {
        return -1;
    }

    // string has no spaces, and is thus one word. so correct
    char *space = strstr(string, " ");
    if (space == NULL) {
        return 0;
    }

    int string_len = strlen(string);

    // determine if string starts and ends with a single quote.
    if (*string != '\'') {
        error("doesn't start with '");
        return -1;
    }

    int ends = -1;
    int index = 1;
    char c[1];
    while (*string++ != '\0') {
        c[0] = *string;
        debug("c is currenlty %s", c);
        if ( strcmp(c, "'") == 0 && index > 1 ) {
            debug("string ends with '");
            ends = 0;
            break;
        }

        index++;
    }

    debug("index is: %d", index);
    if (ends == 0 && string_len == index + 1) {
        return 1;
    }

    // if (*string-- != '\'') {
    //     error("doesn't end with '");
    //     return -1;
    // }

    return -1;
}

COMMANDS_METADATA commands_parse(char *string) {
    COMMANDS_METADATA meta;
    meta.valid = -1;
    meta.valid_args = -1;
    meta.command = "";

    char *dupped = strdup(string);
    meta.whole_args = dupped;

    char *command = NULL;
    char *found;
    //char *string2 = strdup(string);
    while ( (found = strsep(&string, " ")) != NULL ) {
        debug("stepping thru tokens");
        if (command == NULL && strcmp(found, "") == 0) {
            // first token was nothing
            debug("first token was nothing");
            return meta;
        } else if (command == NULL && strcmp(found, "") != 0) {
            // first token wasn't nothing
            command = strdup(found);
            debug("command is: %s", command);
            break;
        } else {
            error("should not be reachable");
            return meta;
        }
    }

    // if reached here, then check if the command is a valid type.
    meta.valid = commands_is_command(command);
    meta.command = command;

    if (string != NULL) {
        meta.valid_args = commands_is_args(string);
        if (meta.valid_args == 1) {
            free(dupped);
            meta.whole_args = remove_quotes(string);
        } else {
            free(dupped);
            debug("dupped2 was mallocd with strlen %ld", strlen(string));
            //char *dupped2 = malloc(sizeof(char) * strlen(string));
            char *dupped2 = strdup(string);
            meta.whole_args = dupped2;
        }
    }

    return meta;
}
