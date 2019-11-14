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
