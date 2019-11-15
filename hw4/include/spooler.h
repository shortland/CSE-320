#ifndef SPOOLER_H
#define SPOOLER_H

// need to free
typedef struct JOB {
    pid_t process;
    uint32_t job_id;
    TASK *task; // need to free
    JOB_STATUS status;
    int exit_status;
    char *task_spec; // need to free
    char *dupp_free; // need to free -holds the address we need to free in addition to the task
} JOB;

typedef struct JOBS_TABLE {
    // pid_t process_starter_pid;
    int enable_spooling;
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
uint32_t spooler_get_runners(void);

/**
 * Set the number of available runners.
 */
void spooler_set_runners(int runners);

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
 * @returns whether spooling is enabled or disabled in the main jobs table
 */
int spooler_jobs_enabled(void);

/**
 * Set the main table spooling status as @param val
 */
void spooler_jobs_set_enable(int val);

/**
 * Set the PID of the starter process
 */
// void spooler_processes_set_starter(pid_t pid);

/**
 * Get the PID of the starter process
 * @returns starter pid
 * @returns -1 if starter is off or doesn't exist (never enabled to begin with.)
 */
// pid_t spooler_processes_get_starter(void);

/**
 * @returns first job of specified status, or NULL if there are none.
 */
JOB *spooler_get_first_by_status(JOB_STATUS status);

/**
 * @returns the job with the specified @param pid
 */
JOB *spooler_get_job_by_pid(pid_t pid);

/**
 *
 */

#endif
