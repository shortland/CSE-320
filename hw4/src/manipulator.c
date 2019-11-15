#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "jobber.h"
#include "task.h"
#include "spooler.h"
#include "shortcuts.h"
#include "processes.h"

int show_all_job_statuses(void) {
    JOBS_TABLE *table = spooler_get_jobs_table();
    if (table->first == NULL) {
        debug("no jobs");

        return 0;
    }

    int job_id = table->first->job_id;
    if ( print_job_status(job_id) != 0) {
        error("error printing job statuses");

        return -1;
    }

    while ( 1 ) {
        table = spooler_next_table(table);

        if (table == NULL) {
            debug("reached end of jobs");

            return 0;
        }

        if (table->first == NULL) {
            debug("reached end of jobs");

            return 0;
        }

        if ( print_job_status(table->first->job_id) != 0) {
            error("error printing job statuses");

            return -1;
        }
    }

    return -1;
}

void change_new_to_waiting(void) {
    JOB *job;

    while ( (job = spooler_get_first_by_status(NEW)) != NULL) {
        debug("found a job of %d that is new!", job->job_id);
        job->status = WAITING;

        printf("job %d status changed: new -> waiting\n", job->job_id);
    }

    debug("no more new to change to waiting");
}

void change_waiting_to_running(void) {
    debug("try to change waiting to running.");
    debug("assumes starting is enabled.");
    debug("must keep track of limit of processes running.");

    JOB *job;
    uint32_t runners = spooler_get_runners();
    pid_t pid;
    while ( ((job = spooler_get_first_by_status(WAITING)) != NULL) && (runners > 0) ) {
        debug("found a job of %d that is waiting!", job->job_id);

        printf("job %d status changed: waiting -> running\n", job->job_id);

        /**
         * processes returns the pid of the new child it forked.
         */
        if ( (pid = processes_spool_new_job(job->job_id)) == -1 ) {
            error("unable to start new job for process?");
        } else {
            job->process = pid;
            job->status = RUNNING;
            spooler_set_runners(spooler_get_runners() - 1);
        }

        runners = spooler_get_runners();
    }

    debug("runners has value: %d", runners);
}

void change_running_to_completed(pid_t pid, int exit_status) {
    debug("getting job from list registered with pid %d", pid);
    JOB *job = spooler_get_job_by_pid(pid);

    if (job == NULL) {
        error("a job with that PID doesn't exist.");
        return;
    }

    job->status = COMPLETED;
    job->exit_status = exit_status;
    job->process = -1;
    debug("successfull changed job (%d) to completed.", job->job_id);

    // update runners
    spooler_set_runners(spooler_get_runners() + 1);

    return;
}
