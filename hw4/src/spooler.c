#include <stdlib.h>

#include "jobber.h"
#include "task.h"

#include "stdint.h"
#include "spooler.h"

// need to free
static JOBS_TABLE *jobs_table;

static uint32_t AVAILABLE_RUNNERS;

JOBS_TABLE *spooler_create_jobs_table(void) {
    jobs_table = malloc(sizeof(JOBS_TABLE));
    jobs_table->first = NULL;
    jobs_table->rest = NULL;
    jobs_table->total = 0;
    return jobs_table;
}

void spooler_free_jobs_table(void) {
    free(jobs_table);
}

uint32_t spooler_available_runners(void) {
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
            error("there are no jobs");

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
