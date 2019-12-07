#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>

#include "client_registry.h"
#include "exchange.h"
#include "trader.h"
#include "debug.h"

#include "csapp.h"
#include "server.h"

#define LISTEN_BACKLOG 128

extern EXCHANGE *exchange;
extern CLIENT_REGISTRY *client_registry;

static void terminate(int status);

/**
 *
 */

/**
 * Server handle SIGHUP cleanly.
 */
void server_sighup_handler(int sig) {
    terminate(EXIT_SUCCESS);
}

/*
 * "Bourse" exchange server.
 *
 * Usage: bourse <port>
 */
int main(int argc, char* argv[]) {
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int opt;
    char *port = NULL;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                if (optarg == NULL) {
                    error("error null optarg given.");

                    exit(EXIT_FAILURE);
                }

                port = optarg;

                break;
            case ':':
                debug("parameter must have a value.");

                break;
            case '?':
                debug("unknown parameter given: %c.", optopt);

                break;
            default:
                debug("no parameter given, terminating server.");

                exit(EXIT_FAILURE);

                break;
        }
    }

    if (port == NULL) {
        error("was not given port");

        exit(EXIT_FAILURE);
    }

    // int port = atoi(port_str);
    // if (port == 0) {
    //     error("invalid port given");

    //     terminate(EXIT_FAILURE);
    // }

    // Perform required initializations of the client_registry,
    // maze, and player modules.
    client_registry = creg_init();
    exchange = exchange_init();
    trader_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function brs_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    int socket;
    errno = 0;

    if ((socket = open_listenfd(port)) == -1) {
        error("error trying to create server on port %s, (errno: %d)", port, errno);

        exit(EXIT_FAILURE);
    } else if (socket == -2) {
        error("error trying to create server on port %s, (getaddrinfo error)", port);

        exit(EXIT_FAILURE);
    } else {
        debug("server socket successfully created on port %s.", port);
    }

    // now listen
    // "check if successful, then for errors." - wise professor
    errno = 0;

    if (listen(socket, LISTEN_BACKLOG) == 0) {
        debug("successfully listening on the socket fd: %d w/port %s", socket, port);
    } else {
        error("unable to listen on the socket fd: %d w/port %s; errno: %d", socket, port, errno);

        exit(EXIT_FAILURE);
    }

    // install signal handler
    struct sigaction signal;
    signal.sa_handler = server_sighup_handler;
    sigaction(SIGHUP, &signal, NULL);

    int *client;
    pthread_t thread;
    // struct sockaddr_un my_addr, peer_addr;
    // socklen_t peer_addr_size;
    while (1) {
        // TODO: free this
        client = Malloc(sizeof(int));
        *client = Accept(socket, NULL, NULL);
        Pthread_create(&thread, NULL, brs_client_service, client);

        // just in-case cleanup (not free'ing it yet, so temp)
        client = NULL;
    }

    // if it's reached, it should be an error
    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    exchange_fini(exchange);
    trader_fini();

    debug("Bourse server terminating");
    exit(status);
}
