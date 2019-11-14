#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "jobber.h"
#include "task.h"
#include "spooler.h"
#include "shortcuts.h"

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
