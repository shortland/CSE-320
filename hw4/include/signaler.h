#ifndef SIGNALER_H
#define SIGNALER_H

void signaler_handler_job_completed(int signal);

int signaler_determine_signal_action(void);

#endif
