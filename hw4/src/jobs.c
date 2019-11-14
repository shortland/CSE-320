/*
 * Job manager for "jobber".
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "jobber.h"
#include "task.h"
#include "spooler.h"

/**
 * I guess create the spooler here.
 * The array/table.
 */
int jobs_init(void) {
    // create the jobs table.
    JOBS_TABLE *jobs_table = spooler_create_jobs_table();

    debug("the table has %d current jobs", jobs_table->total);

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
 *
 */
int jobs_set_enabled(int val) {
    // TO BE IMPLEMENTED
    abort();
}

/**
 *
 */
int jobs_get_enabled() {
    // TO BE IMPLEMENTED
    abort();
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

    spooler_increment_job_count();

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
 *
 */
int job_get_pgid(int jobid) {
    // TO BE IMPLEMENTED
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
 *
 */
int job_get_result(int jobid) {
    // TO BE IMPLEMENTED
    abort();
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
