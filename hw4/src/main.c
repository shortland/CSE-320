#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "jobber.h"
#include "debug.h"
#include "commands.h"

/**
 * "Jobber" job spooler.
 * TODO: argc and argv are not used.
 */
int main(int argc, char *argv[])
{
    /**
     * jobs_init();
     */
    if ( jobs_init() != 0) {
        error("Error initializing spooler");

        return EXIT_FAILURE;
    }

    /**
     * Prompt the user for a jobber command
     */
    char *input_line = "";
    COMMANDS_METADATA meta;
    meta.command = "";

    int exits_with = EXIT_SUCCESS;
    while ( 1 ) {
        input_line = sf_readline("jobber> ");

        // parse the command out
        meta = commands_parse(input_line);

        debug("valid is: %d", meta.valid);
        debug("command is: %s", meta.command);
        debug("whole_args is: %s", meta.whole_args);
        debug("valid_args is: %d", meta.valid_args);

        if ( meta.valid == 0 ) {
            if ( strcmp(meta.command, "quit") == 0 ) {
                free(input_line);
                free(meta.command);
                free(meta.whole_args);

                exits_with = EXIT_SUCCESS;
                break;
            } else if ( strcmp(meta.command, "help") == 0 ) {
                printf("Available commands:\n"
                    "help (0 args) Print this help message\n"
                    "quit (0 args) Quit the program\n"
                    "enable (0 args) Allow jobs to start\n"
                    "disable (0 args) Prevent jobs from starting\n"
                    "spool (1 args) Spool a new job\n"
                    "pause (1 args) Pause a running job\n"
                    "resume (1 args) Resume a paused job\n"
                    "cancel (1 args) Cancel an unfinished job\n"
                    "expunge (1 args) Expunge a finished job\n"
                    "status (1 args) Print the status of a job\n"
                    "jobs (0 args) Print the status of all jobs\n"
                );
            } else if ( strcmp(meta.command, "spool") == 0 ) {
                if (meta.valid_args == -1) {
                    printf("Wrong number of args (given: 0, required: 1) for command 'spool'\n");
                } else { // if (meta.valid_args == 0) {
                    if ( strcmp(meta.whole_args, " ") == 0 || strlen(meta.whole_args) == 0 ) {
                        printf("Wrong number of args (given: 0, required: 1) for command 'spool'\n");
                    } else {
                        int job_id;
                        if ( (job_id = job_create(meta.whole_args)) == -1 ) {
                            error("failed to create job");

                            // TODO: am i supposed to quit the program or something if job_create fails? or try again?
                            free(input_line);
                            free(meta.command);
                            free(meta.whole_args);

                            exits_with = EXIT_FAILURE;
                            break;
                        } else {
                            debug("%ld: job %d created", time(NULL), job_id);
                            debug("%ld: job %d status changed: new -> waiting", time(NULL), job_id);
                        }
                    }
                }
            } else {
                error("TODO: I'm missing a valid command: '%s'?", meta.command);
            }
        } else {
            printf("Unrecognized command: %s\n", meta.command);
        }

        /**
         * At end of while
         */
        free(input_line);
        free(meta.command);
        free(meta.whole_args);
    }

    /**
     * jobs_finit();
     */
    jobs_fini();

    return exits_with;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
