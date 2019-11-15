/*
 * Job manager for "jobber".
 */

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "jobber.h"
#include "task.h"
#include "spooler.h"
#include "processes.h"
#include "signaler.h"
#include "sf_readline.h"

/**
 * I guess create the spooler here.
 * The array/table.
 */
int jobs_init(void) {
    // create the jobs table.
    JOBS_TABLE *jobs_table = spooler_create_jobs_table();
    jobs_table = jobs_table; // TODO:
    debug("the table has %d current jobs", jobs_table->total);

    // set the # of available runners to the # of max runners defined in jobber.h
    spooler_set_runners(MAX_RUNNERS);

    /**
     * Create a SigChld handler that starts new jobs if the SIGCHLD signal means a child finished
     * which results in us having more usable threads (max4) to start more jobs
     */
    struct sigaction sc;
    sc.sa_handler = signaler_handler_job_completed;
    sc.sa_flags = 0;
    sigemptyset(&sc.sa_mask);

    if ( sigaction(SIGCHLD, &sc, NULL) == -1 ) {
        error("Couldn't set SIGCHLD handler");
        return -1;
    }
    // if (signal(SIGCHLD, signaler_handler_job_completed) == SIG_ERR) {
    //     error("SIGCHLD install job_completed signal error");
    //     return -1;
    // }

    sf_set_readline_signal_hook(signaler_determine_signal_action);

    return 0;
}

/**
 * free the jobs table and its jobs... cancel jobs/abort them? read doc.
 */
void jobs_fini(void) {
    // TODO: cancel/abort existing jobs and etc - should happen before freeing jobs_table.

    // free the jobs table.
    spooler_free_jobs_table();

    return;
}

/**
 * Creates a fork process which just checks for new jobs and starts them
 */
int jobs_set_enabled(int val) {
    if (val != 0) {
        debug("enable jobs");
    } else {
        debug("disable jobs");
    }

    int enabled_prev = spooler_jobs_enabled();
    if (enabled_prev != 0 && val != 0) {
        debug("spooling is already enabled.");

        return enabled_prev;
    }

    // else {
    //     debug("spooling appear to be off, so we can start it!");
    // }

    // debug("attempting to enable job starting");
    // pid_t pid;
    // if ( (pid = processes_spool_new_jobs()) == -1) {
    //     error("unable to start process of spooling new jobs");

    //     return enabled_prev;

    //     // we are only supposed to return the original enable status.
    //     // abort();
    // }

    debug("looks like process of spooling new jobs is running!");
    // set the enabled to whatever val is.

    // set the jobs-enabled flag to whatever was passed in to this function
    spooler_jobs_set_enable(val);

    // set the process_starter pid so we can later check that it exited
    // spooler_processes_set_starter(pid);

    return enabled_prev;
}

/**
 * Get whether job-spooling is enabled
 */
int jobs_get_enabled() {
    return spooler_jobs_enabled();
}

/**
 * SHould return the ID of the job on the table.
 * TODO: incomplete, works when only adding jobs. remove breaks it.
 */
int job_create(char *command) {
    int num_jobs;
    if ( (num_jobs = spooler_total_jobs()) == MAX_JOBS) {
        error("exceeds maximum jobs on job table");

        return -1;
    }

    char *dupped[1];
    char *dupped_ts = strdup(command);
    dupped[0] = strdup(command);

    TASK *task = parse_task(dupped);
    if (task == NULL) {
        free(dupped[0]);
        return -1;
    }

    JOB *job = malloc(sizeof(JOB));
    job->job_id = num_jobs;
    job->task = task;
    job->status = NEW;
    // job->exit_status
    job->task_spec = dupped_ts;
    job->dupp_free = dupped[0];

    JOBS_TABLE *table = spooler_get_jobs_table();
    if (num_jobs == 0) {
        table->first = job;
    } else {
        JOBS_TABLE *new_table = spooler_get_empty_jobs_table(table);
        new_table->first = job;
        debug("set the job in next/new table.");
    }

    // TODO:
    sf_job_create(job->job_id);

    spooler_increment_job_count();
    uint32_t cnum_jobs = spooler_total_jobs();
    cnum_jobs = cnum_jobs; // TODO
    debug("the number of jobs is currently: %d", cnum_jobs);

    // if starter spooler is enabled/on then tell it to restart
    // pid_t starter_pid = spooler_processes_get_starter();
    // int enabled_starting = spooler_jobs_enabled();
    // if (starter_pid != -1 && enabled_starting != 0) {
    //     debug("send signal to spool starter to restart.");
    //     // send a signal to starter processes to restart gracefully. (finish tasks first!)
    //     if (kill() != 0) {
    //         error("unable to send signal to process to restart");
    //         return -1;
    //     }
    // }

    return num_jobs; // TODO eventually need to change this. assumes the next job is gonna be free/new one,
}

/**
 *
 */
int job_expunge(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

/**
 *
 */
int job_cancel(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

/**
 *
 */
int job_pause(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

/**
 *
 */
int job_resume(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

/**
 * return the specified jobs pid
 */
int job_get_pgid(int jobid) {
    // JOBS_TABLE *table = spooler_get_specific_jobs_table(jobid);
    // if (table == NULL) {
    //     return -1;
    // }

    // int process = table->first->process;
    // if (process == NULL) {
    //     return -1;
    // }

    // return process;
    abort();
}

/**
 * fail if the job specified jobid is not in the job table.
 */
JOB_STATUS job_get_status(int jobid) {
    debug("attempting to get specific jobs table by it");
    JOBS_TABLE *table = spooler_get_specific_jobs_table(jobid);
    if (table == NULL) {
        return -1;
    }

    return table->first->status;
}

/**
 * return the specified job exit status
 */
int job_get_result(int jobid) {
    JOBS_TABLE *table = spooler_get_specific_jobs_table(jobid);
    if (table == NULL) {
        return -1;
    }

    return table->first->exit_status;
}

/**
 * 
 */
int job_was_canceled(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

/**
 * fail if the specified jobid is not in the job table.
 * the original string the user typed out after spool ...
 */
char *job_get_taskspec(int jobid) {
    JOBS_TABLE *table = spooler_get_specific_jobs_table(jobid);
    if (table == NULL) {
        return NULL;
    }

    return table->first->task_spec;
}
