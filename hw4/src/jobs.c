/*
 * Job manager for "jobber".
 */

#include <stdlib.h>

#include "jobber.h"
#include "task.h"

int jobs_init(void) {
    // TO BE IMPLEMENTED
    abort();
}

void jobs_fini(void) {
    // TO BE IMPLEMENTED
    abort();
}

int jobs_set_enabled(int val) {
    // TO BE IMPLEMENTED
    abort();
}

int jobs_get_enabled() {
    // TO BE IMPLEMENTED
    abort();
}

int job_create(char *command) {
    // TO BE IMPLEMENTED
    abort();
}

int job_expunge(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_cancel(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_pause(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_resume(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_get_pgid(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

JOB_STATUS job_get_status(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_get_result(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

int job_was_canceled(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}

char *job_get_taskspec(int jobid) {
    // TO BE IMPLEMENTED
    abort();
}
