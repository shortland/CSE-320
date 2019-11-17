#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>

#include "debug.h"
#include "jobber.h"
#include "task.h"

#include "stdint.h"
#include "spooler.h"
#include "processes.h"

volatile sig_atomic_t canceled;

void handler_sigkill(int signal) {
    error("received SIGKILL, 'cleanly' stopping process: '%d'", getpid());
    // TODO - way of telling parent we stopped, parent needs to read this exit status?
    canceled = 1;
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

    int pipe_amt = 0;
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
        // if (signal(SIGKILL, handler_sigkill) == SIG_ERR) {
        //     error("signal-sigint install error");

        //     return -1;
        // }

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
            if ( (pid_pipeline_master = fork()) == 0 ) {
                // Master Pipeline Process, there should be as many of these for a TASK as there are pipelines.
                // pipeline ; pipeline ; pipeline
                debug("hello from pipline master process. %d is my pid", getpid());

                /**
                 * create child foreach command in the pipeline.
                 */
                COMMAND_LIST *commands = pipeline->commands;

                /**
                 * The initial file to read input from
                 */
                char *input_path = pipeline->input_path;
                int fd_in = -1;
                if (input_path != NULL) {
                    fd_in = open(input_path, O_RDONLY);
                    if (fd_in == -1) {
                        error("unable to open input file.");
                        // abort?
                        exit(-1);
                    }
                }

                /**
                 * The final file to output data to;
                 * if it's not the last command, then should pipe data to next process...
                 */
                char *output_path = pipeline->output_path;
                int last_fd_out = -1;
                if (output_path != NULL) {
                    debug("found output path, trying to open it.");
                    last_fd_out = open(output_path, O_WRONLY | O_TRUNC | O_CREAT, 00777);
                    // error("the fdout is currently: %d", last_fd_out);
                    if (last_fd_out == -1) {
                        error("unable to open last output file.");
                        // abort?
                        exit(-1);
                    }
                }

                /**
                 * The pipe FDs, for intermediary commands
                 */
                int pipe_fds[2];
                int read_pipe_prev = -1;

                int command_num = 0;
                int child_status;
                pid_t command_pid;
                while (commands != NULL) {
                    COMMAND *command = commands->first;

                    /**
                     * Create the pipes
                     */
                    if ( pipe(pipe_fds) == -1) {
                        error("unable to create pipes");
                        exit(-1);
                    }

                    /**
                     * Spawn the next command child
                     */
                    if ( (command_pid = fork()) == 0 ) {
                        // if is first command
                        if (command_num == 0) {
                            debug("this is the first child command, so it gets its input via fd_in, if not null");

                            if (fd_in != -1) {
                                debug("There is a fd_in for the first command, so attempt to use it.");

                                if (dup2(fd_in, 0) == -1) {
                                    error("unable to change first command fd to fd_in");
                                    exit(-1);
                                }
                                close(fd_in);
                            }
                        } else {
                            // if it's not the first command (it's any other than first)
                            // get it's input from the pipe
                            debug("this is not the first command, so replace its input from pipe in");

                            if (read_pipe_prev != -1) {
                                if (dup2(read_pipe_prev, 0) == -1) {
                                    error("unable to change non-first command input to pipe in");
                                    exit(-1);
                                }
                            }
                            close(read_pipe_prev);
                        }

                        // if is last command
                        if (commands->rest == NULL) {
                            debug("this is the last child command, so it sets its output via last_fd_out, if not null");

                            if (last_fd_out != -1) {
                                debug("There is a fd_out for the last command, so attempt to use it.");

                                if (dup2(last_fd_out, 1) == -1) {
                                    error("unable to change last command fd to last_fd_out");
                                    exit(-1);
                                }
                                // error("changed fdout to %d", last_fd_out);
                                close(last_fd_out);
                            }
                        } else {
                            // if it's not the last command (it's any other than last)
                            // pipe its output to the next command,
                            debug("this is not the last command, so replace its output with pipe out");

                            if (dup2(pipe_fds[1], 1) == -1) {
                                error("unable to change non-last command fd to pipe out");
                                exit(-1);
                            }
                            close(pipe_fds[1]);
                        }

                        // create a dynamically allocated array for the args of the task.
                        // first put the original command into the array
                        char **command_args = ((char **) malloc(1 * sizeof(char *)));
                        command_args[0] = malloc((strlen(command->words->first) + 1) * sizeof(char));
                        command_args[0] = command->words->first;

                        // now, dynamically increase the size of the array.
                        WORD_LIST *list = command->words->rest;
                        int count = 2;
                        while (list != NULL) {
                            command_args = ((char **) realloc(command_args, count * sizeof(char *)));
                            command_args[count - 1] = malloc( (strlen(list->first) + 1) * sizeof(char));
                            command_args[count - 1] = list->first;

                            warn("that command is now %s", command_args[count - 1]);

                            list = list->rest;
                            count++;
                        }

                        // add the last one which must be null,
                        command_args = ((char **) realloc(command_args, count * sizeof(char *)));
                        command_args[count - 1] = malloc(sizeof(NULL));
                        command_args[count - 1] = NULL;

                        errno = 0;
                        debug("command:%s", command->words->first);
                        if (execvp(command->words->first, command_args) == -1) {
                            error("execvp failed to execute %d", errno);
                            exit(-1);
                        }

                        exit(0);
                    } else if (command_pid == -1) {
                        error("unable to spawn child command");
                        exit(-1);
                    } else {
                        // parent command
                        // wait for child to finish by waiting for its PID (command_pid)
                        debug("pipeline master, waiting for child to finish");
                        waitpid(command_pid, &child_status, 0);
                        debug("child (%d) seems to have reaped %d", command_pid, getpid());
                        // ...
                        // parent here, has the output of the piped command from child.
                        // parent should make that data it reads, the input to the next command.
                        read_pipe_prev = pipe_fds[0];
                        // ...
                        // close the writing pipe in the parent always
                        close(pipe_fds[1]);

                        /**
                         * Update stuff for the next command child
                         */
                        commands = commands->rest;
                        command_num++;
                    }
                }

                exit(0);
            } else if (pid_pipeline_master == -1) {
                error("unable to make pipline master");
                exit(-1);
            } else {
                // pipeline parent master (aka processes master
                pipe_amt++;
            }

            // at end
            pipelines = pipelines->rest;
        } // end while pipelines

        // still process master thread (child)
        // wait for child pipeline masters

        if (signal(SIGINT, handler_sigkill) == SIG_ERR) {
            error("signal (cancel) install error");

            return -1;
        }

        check_pipe_amt:
        while ( canceled == 0 && pipe_amt != 0 ) {
            // error("waiting for pipeline children to finish (children of this task - master process %d)", getpid());
            int pipe_line_status;
            pid_t pid = waitpid(0, &pipe_line_status, WNOHANG);

            if (pid <= 0) {
                // error("error waiting for pipeline to finish %d", pid);
                goto check_pipe_amt;
            }

            warn("pipline process pid:%d exited with status: %d", pid, pipe_line_status);
            pipe_amt--;
        }

        // exit the master thread
        exit(0);
    }
    // main thread

    debug("processes_spooler() checking to start new tasks - parent pid: %d (spawned child: %d)", getpid(), pid);
    return pid;
}
