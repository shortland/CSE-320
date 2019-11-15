#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#include "signaler.h"
#include "jobber.h"
#include "manipulator.h"

volatile sig_atomic_t job_completed;
// static int job_completed;

void signaler_handler_job_completed(int signal) {
    debug("signaller job_completed was called!");

    job_completed = 1;
    return;
}

int signaler_determine_signal_action(void) {
    debug("determine action (& do) from flags...");
    warn("job completed is: %d", job_completed);

    job_comp:
    if ( job_completed != 0 ) {
        job_completed = 0;
        debug("a job(s) completed!");

        /**
         * signals get smooshed if we don't read them fast enough.
         * so we do this.
         */
        while ( 1 ) {
            debug("checking to reap children.");

            int status;
            pid_t wpid = waitpid(-1, &status, WNOHANG);

            if (wpid <= 0) {
                debug("no more pids to wait for.");

                goto job_comp;
            }

            if (WIFEXITED(status)) {
                debug("Child %d terminated with exit status %d\n", wpid, WEXITSTATUS(status));

                if (status == 0) {
                    // change from running to completed.
                    change_running_to_completed(wpid, status);
                } else {
                    error("unrecognized exit status - abnormal? %d", status);
                }
            }  else {
                error("no exit status - abnormal? pid: (%d)", wpid);
            }
        }

        if ( jobs_get_enabled() != 0 ) {
            debug("job starting is enabled.");

            change_waiting_to_running();
        }
    }

    if ( jobs_get_enabled() != 0 ) {
        debug("job starting is enabled.");

        change_waiting_to_running();
    }

    return 0;
}
