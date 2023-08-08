#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <stdbool.h>

#include "talk.h"
#include "parser.h"
#include "process.h"
#include "queue.h"

#define OPTIONS              "t:l:"
#define BUF_SIZE             4096
#define DEFAULT_THREAD_COUNT 4

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_dispatcher = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_consumers = PTHREAD_COND_INITIALIZER;
Queue *requests;
int shutdown_threads;
int worker_threads;
pthread_t *all_threads;
static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);

// Converts a string to an 16 bits unsigned integer.
// Returns 0 if the string is malformed or out of the range.
static size_t strtouint16(char number[]) {
    char *last;
    long num = strtol(number, &last, 10);
    if (num <= 0 || num > UINT16_MAX || *last != '\0') {
        return 0;
    }
    return num;
}

// Creates a socket for listening for connections.
// Closes the program and prints an error message on error.
static int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 128) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

static void handle_connection(Read_request *rq) {
    // Start by reading the request.
    Req_struct *req = NULL;
    // struct pollfd poll_fd;
    // poll_fd.fd = rq->connfd;
    // poll_fd.events = POLLIN;
    // int to_read = poll(&poll_fd, 1, 1000);

    // If we have bytes to read and we have read a full request
    if (recv_request(rq) != -1) {
        // Next we want to parse the request
        // printf("[%d]: Read req :[%s]\n", rq->connfd, rq->request);
        req = parse_request(rq->request, rq->request_length, rq->message_start, rq->connfd);
        // printf("[%d]: Parsed req\n", rq->connfd);

        // --------- CRITICAL REGION START ------------
        if (req) {
            // Process the request.
            process_request(req, rq->connfd, logfile);
            // printf("[%d]: Processed req\n", rq->connfd);
            // LOG("%s,/%s,%d,%d\n", req->method, req->uri, status_code, req->request_id);
            // fflush(logfile);
            // Delete the read request
            destroy_req(&req);
        } else {
            send_response(rq->connfd, 500, 0);
        }
        // --------- CRITICAL REGION END ------------
        // close(rq->connfd);
        // destroy_read_request(&rq);
        // printf("[%d]: Finished req\n", rq->connfd);
    }

    return;
}

static void *wait_handle_connection() {
    while (1) {
        // Get a request from the queue
        // Lock mutex
        pthread_mutex_lock(&mutex);
        // Wait for a request to come through
        while (queue_empty(requests)) {
            if (pthread_cond_wait(&cond_consumers, &mutex)) {
                perror("!!! Error !!!\n");
            }
            if (shutdown_threads) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
        }
        int connfd = -1;
        // Grab the request
        dequeue(requests, &connfd);
        // printf("Grabbing from queue : [%d]\n", rq->connfd);
        // print_queue(requests);
        // Signal dispatcher there is room in queue to add
        pthread_cond_signal(&cond_dispatcher);
        // Can now unlock
        pthread_mutex_unlock(&mutex);
        // Process the request
        Read_request *rq = create_read_request(connfd);
        handle_connection(rq);
        close(rq->connfd);
        destroy_read_request(&rq);
    }
    return NULL;
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        warnx("received SIGTERM or SIGINT");
        // set shutdown flag and wake all threads up
        shutdown_threads = 1;
        pthread_cond_broadcast(&cond_consumers);
        for (int i = 0; i < worker_threads; i += 1) {
            pthread_join(all_threads[i], NULL);
        }
        fclose(logfile);
        queue_delete(&requests);
        free(all_threads);
        exit(EXIT_SUCCESS);
    }
}

static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        errx(EXIT_FAILURE, "bad port number: %s", argv[1]);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    int listenfd = create_listen_socket(port);

    // Create a queue to hold all requests
    requests = queue_create(512);
    shutdown_threads = 0;
    worker_threads = threads;

    // Place to hold all threads
    all_threads = (pthread_t *) malloc(threads * sizeof(pthread_t));

    // sigset_t *set = NULL;
    // sigemptyset(set);
    // sigaddset(set, SIGINT);
    // sigaddset(set, SIGTERM);
    // pthread_sigmask(SIG_BLOCK, set, NULL);

    // Create all needed worker threads
    for (int i = 0; i < threads; i += 1) {
        pthread_create(&all_threads[i], NULL, wait_handle_connection, NULL);
    }

    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }
        // Add to the queue and tell others that there is work to be done
        pthread_mutex_lock(&mutex);
        while (queue_full(requests)) {
            if (pthread_cond_wait(&cond_dispatcher, &mutex)) {
                perror("!!! Error !!!\n");
            }
        }
        enqueue(requests, connfd);
        // printf("Adding to queue\n");
        // print_queue(requests);
        pthread_cond_signal(&cond_consumers);
        pthread_mutex_unlock(&mutex);
    }

    return EXIT_SUCCESS;
}
