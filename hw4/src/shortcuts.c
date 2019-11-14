#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "jobber.h"
#include "task.h"
#include "spooler.h"
#include "shortcuts.h"

int print_job_status(int job_id) {
    JOB_STATUS status = job_get_status(job_id);
    if (status == -1) {
        error("invalid job choice");

        return -1;
    }

    char *taskspec = job_get_taskspec(job_id);
    if (taskspec == NULL) {
        error("invalid job choice");

        return -1;
    }

    printf("job %d [%s]: %s\n", job_id, job_status_names[status], taskspec);
    return 0;
}
