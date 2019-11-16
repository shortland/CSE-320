#include <stdlib.h>

#include "jobber.h"
#include "task.h"

#include "stdint.h"
#include "spooler.h"
#include "processes.h"

// need to free
static JOBS_TABLE *jobs_table;

static uint32_t AVAILABLE_RUNNERS;

JOBS_TABLE *spooler_create_jobs_table(void) {
    jobs_table = malloc(sizeof(JOBS_TABLE));
    jobs_table->first = NULL;
    jobs_table->rest = NULL;
    jobs_table->total = 0;
    jobs_table->enable_spooling = 0; // spooling is off initially.
    // jobs_table->process_starter_pid = -1;
    return jobs_table;
}

void spooler_free_jobs_table(void) {
    free(jobs_table);
    return;
}

void spooler_set_runners(int runners) {
    debug("setting to %d", runners);
    AVAILABLE_RUNNERS = runners;
    return;
}

uint32_t spooler_get_runners(void) {
    return AVAILABLE_RUNNERS;
}

uint32_t spooler_total_jobs(void) {
    return jobs_table->total;
}

void spooler_increment_job_count(void) {
    jobs_table->total++;
}

JOBS_TABLE *spooler_get_jobs_table(void) {
    return jobs_table;
}

int spooler_jobs_enabled(void) {
    debug("attempting to get jobs table enable-status");
    JOBS_TABLE *table = spooler_get_jobs_table();
    return table->enable_spooling;
}

void spooler_jobs_set_enable(int val) {
    debug("attempting to set jobs table enable-status to '%d'", val);
    JOBS_TABLE *table = spooler_get_jobs_table();
    table->enable_spooling = val;
    return;
}

// given an existing, filled job table.
// should basically always be given main jobs table.
JOBS_TABLE *spooler_get_empty_jobs_table(JOBS_TABLE *table) {
    if (table->rest == NULL) {
        JOBS_TABLE *new_table = malloc(sizeof(JOBS_TABLE));
        new_table->first = NULL;
        new_table->rest = NULL;
        // jobs_table->total = 0; // we don't care about the total in other tables besides the main one.
        table->rest = new_table; //maybe?
        return new_table;
    } else {
        return spooler_get_empty_jobs_table(table->rest);
    }
}

JOBS_TABLE *spooler_get_specific_jobs_table(uint32_t job_id) {
    debug("attempting to get specific jobs table by it");
    JOBS_TABLE *table = spooler_get_jobs_table();

    while (1) {
        if (table->first == NULL) {
            debug("there are no jobs");

            return NULL;
        }

        if (table->first->job_id == job_id) {
            debug("found job table with job_id %d", job_id);
            return table;
        } else {
            if (table->rest == NULL) {
                break;
            }

            table = table->rest;
        }
    }

    error("unable to find job table with job with job_id %d", job_id);
    return NULL;
}

JOBS_TABLE *spooler_next_table(JOBS_TABLE *table) {
    if (table == NULL) {
        return NULL;
    }

    return table->rest;
}

JOB *spooler_get_first_by_status(JOB_STATUS status) {
    debug("attempting to return first job of status [%s]", job_status_names[status]);
    JOBS_TABLE *table = spooler_get_jobs_table();

    while ( 1 ) {
        if (table->first == NULL) {
            debug("there are no jobs yet");

            return NULL;
        }

        if (table->first->status == status) {
            debug("found job_id (%d) with that status", table->first->job_id);

            return table->first;
        } else {
            if (table->rest == NULL) {
                break;
            }

            table = table->rest;
        }
    }

    debug("unable to find any jobs of [%s] status", job_status_names[status]);
    return NULL;
}

JOB *spooler_get_job_by_pid(pid_t pid) {
    debug("attempting to return job with pid [%d]", pid);
    JOBS_TABLE *table = spooler_get_jobs_table();

    while ( 1 ) {
        if (table->first == NULL) {
            debug("there are no jobs yet");

            return NULL;
        }

        if (table->first->process == pid) {
            debug("found job_id (%d) with that pid", table->first->job_id);

            return table->first;
        } else {
            if (table->rest == NULL) {
                break;
            }

            table = table->rest;
        }
    }

    debug("unable to find any jobs with pid [%d]", pid);
    return NULL;
}

JOB *spooler_get_job_by_job_id(uint32_t job_id) {
    debug("attempting to return job with job_id [%d]", job_id);
    JOBS_TABLE *table = spooler_get_jobs_table();

    while ( 1 ) {
        if (table->first == NULL) {
            debug("there are no jobs yet");

            return NULL;
        }

        if (table->first->job_id == job_id) {
            debug("found job_id (%d) that matches", table->first->job_id);

            return table->first;
        } else {
            if (table->rest == NULL) {
                break;
            }

            table = table->rest;
        }
    }

    debug("unable to find any jobs with job_id [%d]", job_id);
    return NULL;
}
