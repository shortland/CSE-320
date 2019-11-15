#ifndef MANIPULATOR_H
#define MANIPULATOR_H

int show_all_job_statuses(void);

void change_new_to_waiting(void);

void change_waiting_to_running(void);

void change_running_to_completed(pid_t pid, int exit_status);

#endif
