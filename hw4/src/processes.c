#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#include "debug.h"
#include "jobber.h"
#include "task.h"

#include "stdint.h"
#include "spooler.h"
#include "processes.h"

void handler_sigint(int signal) {
    debug("received sigint, 'cleanly' stopping process: '%d'", getpid());
    // TODO - way of telling parent we stopped, parent needs to read this exit status?

    exit(0);
}

pid_t processes_spool_new_job(int job_id) {
    // create a child process that just checks for a signal to imply that it should start handling jobs.
    JOBS_TABLE *table = spooler_get_specific_jobs_table(job_id);
    if (table == NULL) {
        error("no job exists with the job_id: %d", job_id);

        return -1;
    }

    // so we don't need to reference the job by table->first everywhere.
    JOB *job = table->first;

    pid_t pid;
    /**
     * This creates a runner process for a given job.
     */
    if ((pid = fork()) == 0) {
        debug("child created, pid: %d", getpid());

        /**
         * Set the process group id to the current process id.
         */
        if (setpgid(getpid(), getpid()) != 0) {
            error("unable to set the process group id");

            return -1;
        }

        sf_job_start(job->job_id, getpid());

        /**
         * Install the sigint-handler for the child
         */
        if (signal(SIGINT, handler_sigint) == SIG_ERR) {
            error("signal-sigint install error");

            return -1;
        }

        /**
         * task->pipelines->first->commands->first->words->first->rest
         * task->pipelines->first->input_path
         * task->pipelines->first->output_path
         *
         * task->pipelines->first->commands->rest
         *
         * task->pipelines->rest...
         */
        PIPELINE_LIST *pipelines = job->task->pipelines;
        while (pipelines != NULL) {
            PIPELINE *pipeline = pipelines->first;

            warn("got a pipeline!");
            pid_t pid_pipeline_master;
            if ( (pid_pipeline_master = fork()) == 0) {
                // Master Pipeline Process, there should be as many of these for a TASK as there are pipelines.
                // pipeline ; pipeline ; pipeline
                debug("hello from pipline master process.");

                // create child foreach command in the pipeline.
                COMMAND_LIST *commands = pipeline->commands;
                char *input_path = pipeline->input_path;
                input_path = input_path; // TODO:
                char *output_path = pipeline->output_path;
                output_path = output_path; //TODO:
                while (commands != NULL) {
                    COMMAND *command = commands->first;

                    command = command;
                    warn("we have a command!");

                    commands = commands->rest;
                }

                exit(0);
            }

            // at end
            pipelines = pipelines->rest;
        }

        // the stuff this child is supposed to do is:
        debug("The stuff I'm supposed to do: %s", table->first->task_spec);

        // check for new jobs that need to be started
        sleep(2);

        // exit normally
        exit(0);
    }

    debug("processes_spooler() checking to start new tasks - parent pid: %d (spawned child: %d)", getpid(), pid);
    return pid;
}
