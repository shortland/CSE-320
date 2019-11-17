#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "jobber.h"
#include "debug.h"
#include "commands.h"
#include "shortcuts.h"
#include "manipulator.h"

// my first ever macro :o
#define BAD_ARGS(HAS, COMMAND) printf("Wrong number of args (given: " #HAS ", required: 1) for command '" #COMMAND "'\n");

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
    int d = 1;
    int enable_jobs;

    while ( 1 ) {
        enable_jobs = -1;

        // hacky block that's only reachable via goto, so we can reprompt and free.
        // don't need to do this, but I rather than re-write the 3 free-s over and over.
        if ( d == 0 ) {
            freereprompt:
            debug("only reachable when error and want to free");

            free(input_line);
            free(meta.command);
            free(meta.whole_args);
        }

        reprompt:
        input_line = sf_readline("jobber> ");
        if ( input_line == NULL ) {
            return EXIT_SUCCESS;
        }

        if ( strlen(input_line) == 0 ) {
            free(input_line);
            goto reprompt;
        }

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

                            goto freereprompt;
                        }

                        // printf("%ld: job %d created\n", time(NULL), job_id);
                        // printf("%ld: job %d status changed: new -> waiting\n", time(NULL), job_id);
                    }
                }
            } else if ( strcmp(meta.command, "status") == 0 ) {
                if (meta.valid_args == -1) {
                    BAD_ARGS(0, "status");

                    goto freereprompt;
                }

                if ( strcmp(meta.whole_args, " ") == 0 || strlen(meta.whole_args) == 0 ) {
                    BAD_ARGS(0, "status");

                    goto freereprompt;
                }

                int job_id = atoi(meta.whole_args);
                debug("the job id we are checing is %d", job_id);

                if ( print_job_status(job_id) != 0) {
                    error("error printing job statuses");

                    goto freereprompt;
                }
            } else if ( strcmp(meta.command, "jobs") == 0 ) {
                if ( show_all_job_statuses() != 0) {
                    error("error printing job statuses");

                    goto freereprompt;
                }
            } else if ( strcmp(meta.command, "enable") == 0 ) {
                debug("should enable jobs");
                enable_jobs = 0;
            } else if ( strcmp(meta.command, "disable") == 0 ) {
                debug("should disable jobs");
                enable_jobs = 1;
            } else if ( strcmp(meta.command, "pause") == 0 ) {
                debug("should pause job");
                if (meta.valid_args == -1) {
                    BAD_ARGS(0, "pause");

                    goto freereprompt;
                }

                if ( strcmp(meta.whole_args, " ") == 0 || strlen(meta.whole_args) == 0 ) {
                    BAD_ARGS(0, "pause");

                    goto freereprompt;
                }

                int job_id = atoi(meta.whole_args);
                if (job_pause(job_id) == -1) {
                    printf("Cannot pause a job with that id.\n");
                    goto freereprompt;
                } else {
                    printf("job %d changed to PAUSED\n", job_id);
                }
            } else if ( strcmp(meta.command, "resume") == 0 ) {
                debug("should resume job");
                if (meta.valid_args == -1) {
                    BAD_ARGS(0, "resume");

                    goto freereprompt;
                }

                if ( strcmp(meta.whole_args, " ") == 0 || strlen(meta.whole_args) == 0 ) {
                    BAD_ARGS(0, "resume");

                    goto freereprompt;
                }

                int job_id = atoi(meta.whole_args);
                if (job_resume(job_id) == -1) {
                    printf("Cannot resume a job with that id.\n");
                    goto freereprompt;
                } else {
                    printf("job %d changed to RUNNING\n", job_id);
                }
            } else if ( strcmp(meta.command, "cancel") == 0 ) {
                debug("should cancel job");
                if (meta.valid_args == -1) {
                    BAD_ARGS(0, "cancel");

                    goto freereprompt;
                }

                if ( strcmp(meta.whole_args, " ") == 0 || strlen(meta.whole_args) == 0 ) {
                    BAD_ARGS(0, "cancel");

                    goto freereprompt;
                }

                int job_id = atoi(meta.whole_args);
                if (job_cancel(job_id) == -1) {
                    printf("Cannot cancel a job with that id.\n");
                    goto freereprompt;
                } else {
                    printf("job %d changed to CANCELED\n", job_id);
                }
            } else if ( strcmp(meta.command, "expunge") == 0 ) {
                debug("should expunge job");
                if (meta.valid_args == -1) {
                    BAD_ARGS(0, "expunge");

                    goto freereprompt;
                }

                if ( strcmp(meta.whole_args, " ") == 0 || strlen(meta.whole_args) == 0 ) {
                    BAD_ARGS(0, "expunge");

                    goto freereprompt;
                }

                int job_id = atoi(meta.whole_args);
                if (job_expunge(job_id) == -1) {
                    printf("Cannot expunge a job with that id %d.\n", job_id);
                    goto freereprompt;
                } else {
                    printf("job %d was expunged.\n", job_id);
                }
            } else if ( strcmp(meta.command, "TMP") == 0 ) {
                if (meta.valid_args == -1) {
                    BAD_ARGS(0, "TMP");

                    goto freereprompt;
                }

                if ( strcmp(meta.whole_args, " ") == 0 || strlen(meta.whole_args) == 0 ) {
                    BAD_ARGS(0, "TMP");

                    goto freereprompt;
                }

                // now do stuff
            } else {
                error("TODO: I'm missing a valid command: '%s'?", meta.command);
            }
        } else {
            printf("Unrecognized command: %s\n", meta.command);
        }

        /**
         * Change jobs that were newly created to "waiting" status
         */
        change_new_to_waiting();

        /**
         * At end of while, enable jobs if 0, disable if 1, leave alone otherwise (-1)
         */
        if ( enable_jobs == 0 ) {
            debug("enabling jobs");
            int enable_was;
            if ( (enable_was = jobs_set_enabled(1)) == 0) {
                printf("starting jobs was off. now should be on.\n");
            } else {
                printf("starting jobs was already on.\n");
            }
        } else if ( enable_jobs == 1 ) {
            debug("stopping jobs");
            int enable_was;
            if ( (enable_was = jobs_set_enabled(0)) == 0) {
                printf("starting jobs was already off. should still be off.\n");
            } else {
                printf("starting jobs was on. should now be off.\n");
            }
        } else {
            // enable_jobs is default value of -1; don't do anything.
        }

        // /**
        //  * At the end of any user input,
        //  * Start any jobs if starter says we can
        // NOTE: moving this into signaler, into the main callback function...
        //  */
        // if ( jobs_get_enabled() != 0 ) {
        //     debug("processes_spool_new_job");

        //     // change_waiting_to_
        //     // this function should fork for EACH job it starts.
        //     // processes_spool_new_job();
        // }

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
