#ifndef SPOOLER_H
#define SPOOLER_H

// need to free
typedef struct JOB {
    uint32_t job_id;
    TASK *task; // need to free
    JOB_STATUS status;
    int exit_status;
    char *task_spec; // need to free
    char *dupp_free; // need to free -holds the address we need to free in addition to the task
} JOB;

typedef struct JOBS_TABLE {
    uint32_t total;
    JOB *first;
    struct JOBS_TABLE *rest;
} JOBS_TABLE;

/**
 * Create the Spooler "Jobs Table"
 */
JOBS_TABLE *spooler_create_jobs_table(void);

/**
 * Free the jobs table.
 */
void spooler_free_jobs_table(void);

/**
 * Get the number of available runners.
 */
uint32_t spooler_available_runners(void);

/**
 * Get the current total number of jobs
 */
uint32_t spooler_total_jobs(void);

/**
 * Increment the total job count.
 */
void spooler_increment_job_count(void);

/**
 * @returns the main jobs table.
 */
JOBS_TABLE *spooler_get_jobs_table(void);

/**
 * @returns creates a new empty jobs table, linked to @param table or its first empty rest;
 */
JOBS_TABLE *spooler_get_empty_jobs_table(JOBS_TABLE *table);

/**
 * @returns the JOB_TABLE that contains the specified job with job_id, returns NULL if not exists.
 */
JOBS_TABLE *spooler_get_specific_jobs_table(uint32_t job_id);

/**
 * @returns next table, null if none or if current table is null
 */
JOBS_TABLE *spooler_next_table(JOBS_TABLE *table);

/**
 *
 */

#endif
