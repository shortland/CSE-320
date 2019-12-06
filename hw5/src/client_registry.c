#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_registry.h"
#include "csapp.h"
#include "debug.h"

struct client_registry {
    uint32_t client_count;
    int *fds;
    // pthread_t *tid;

    pthread_mutex_t count_lock;
};

CLIENT_REGISTRY *creg_init() {
    CLIENT_REGISTRY *cr;

    // create space for the client registry
    cr = Malloc(sizeof(*cr));
    cr->client_count = 0;

    // create space for a first client fd
    cr->fds = (int *)malloc(1 * sizeof(int));

    if (pthread_mutex_init(&(cr->count_lock), NULL) != 0) {
        error("create mutex lock counter error");

        return NULL;
    }

    return cr;
}

void creg_fini(CLIENT_REGISTRY *cr) {
    // maybe?
    pthread_mutex_lock(&(cr->count_lock));


    debug("destroying mutext lock");
    pthread_mutex_destroy(&(cr->count_lock));

    // free the client registry
    debug("freeing client registry");
    free(cr);
}

int creg_register(CLIENT_REGISTRY *cr, int fd) {
    pthread_mutex_lock(&(cr->count_lock));

    // set the new fd to the current index
    cr->fds[cr->client_count] = fd;

    // increment index for next time
    cr->client_count++;
    // create more space for the index for next time
    if (realloc(cr->fds, (cr->client_count + 1) * sizeof(int)) == NULL) {
        error("unable to realloc the fds array in client registry (grow)");

        return -1;
    }

    pthread_mutex_unlock(&(cr->count_lock));

    // success
    debug("successfully registered a client with fd %d", fd);

    return 0;
}

int creg_unregister(CLIENT_REGISTRY *cr, int fd) {
    pthread_mutex_lock(&(cr->count_lock));

    // need to push array downwards...
    // resmush array

    // 4, 6, 9, 1
    // 4, x, 9, 1
    // 4, 9, 1
    // basically removes the fd from array
    int found = -1;
    for (int i = 0; i < cr->client_count; ++i) {
        if (cr->fds[i] == fd) {
            debug("found index (%d) at which to remove fd: %d", i, fd);
            cr->fds[i] = -1;

            // close the file descriptor
            if (close(cr->fds[i]) == -1) {
                warn("unable to close file descriptor, might've already closed.");

                // pthread_mutex_unlock(&(cr->count_lock));
                // return -1;
            }

            found = 0;
            break;
        }
    }

    // if the fd was found and removed, then look for the -1 and then shift everything down
    if (found == 0) {
        for (int i = 0; i < cr->client_count; ++i) {
            if (cr->fds[i] == -1) {
                debug("found old index (%d) at which fd was removed: %d", i, fd);

                for (int j = i; j < cr->client_count; ++j) {
                    if (j + 1 < cr->client_count) {
                        cr->fds[j] = cr->fds[j + 1];
                    }
                }

                break;
            }
        }
    }

    // now that fds array is resorted correctly, we can resize and shrink counter and array
    if (cr->client_count == 1) {
        debug("not shrinking cus there's only 1 slot left anyways. we can still countdown tho");
    } else {
        if (realloc(cr->fds, (cr->client_count - 1) * sizeof(int)) == NULL) {
            error("unable to realloc the fds array in client registry (shrink)");

            pthread_mutex_unlock(&(cr->count_lock));
            return -1;
        }
    }


    // finally, shrink the count
    cr->client_count--;

    // then unlock mutex
    pthread_mutex_unlock(&(cr->count_lock));

    // success
    debug("successfully unregistered a client with fd %d", fd);

    return 0;
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
    waiting_unlock:
    pthread_mutex_lock(&(cr->count_lock));

    if (cr->client_count == 0) {
        debug("client count reached 0");

        pthread_mutex_unlock(&(cr->count_lock));
        return;
    } else {
        debug("waiting for count to reach 0");

        goto waiting_unlock;
    }

    pthread_mutex_unlock(&(cr->count_lock));
    return;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {

    return;
}
