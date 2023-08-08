#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>

#include "parser.h"
#include "io.h"

#define GET_METHOD    "GET"
#define PUT_METHOD    "PUT"
#define APPEND_METHOD "APPEND"
#define LOG(...)      fprintf(logfile, __VA_ARGS__);

int process_get_request(Req_struct *request, int connfd, FILE *logfile) {
    // First we must open theh file.
    int ret_val = 500;
    int error = 0;
    request->fd = open_file(request->uri, O_RDONLY, &error, 0, 0, 0);
    if (error == 0) {
        flock(request->fd, LOCK_SH);
        char read_buf[REQUEST_LENGTH] = { 0 };
        // Check to see if we can read
        if (read(request->fd, read_buf, 0) == 0) {
            // Send response of OK with a message length
            struct stat buf;
            fstat(request->fd, &buf);
            // Get the file size
            int file_size = *((int *) (&buf.st_size));
            ret_val = 200;
            send_response(connfd, 200, file_size);
            // Read and respond.
            while (1) {
                // Read some bytes
                int bytes_read = read_bytes(request->fd, read_buf, REQUEST_LENGTH, &error);
                // If error um what do i do?
                if (error) {
                    // Todo what here?
                    printf("UHOH: [%d]\n", error);
                    break;
                } else {
                    // If we have read some bytes write them to connfd
                    if (bytes_read) {
                        write_bytes(connfd, read_buf, bytes_read);
                        memset(read_buf, 0, REQUEST_LENGTH * sizeof(char));
                    } else {
                        // If we havent read bytes weve reached the end
                        break;
                    }
                }
            }
        } else {
            ret_val = 403;
            send_response(connfd, 403, 0);
        }
        LOG("%s,/%s,%d,%d\n", request->method, request->uri, ret_val, request->request_id);
        fflush(logfile);
        flock(request->fd, LOCK_UN);
        close(request->fd);
        // close(fd);
    } else if (error == 2) {
        ret_val = 404;
        send_response(connfd, 404, 0);
    } else if (error == 13) {
        ret_val = 403;
        send_response(connfd, 403, 0);
    } else if (error == 20) {
        ret_val = 500;
        send_response(connfd, 500, 0);
    }
    return ret_val;
}

int process_put_request(Req_struct *request, int connfd, FILE *logfile) {
    int ret_val = 500;
    int error = 0;
    request->fd = open_file(request->uri, O_WRONLY, &error, 0, 0, 0);
    if (error == 0) {
        flock(request->fd, LOCK_EX);
        ftruncate(request->fd, 0);
        // Write to the file based on the message length.
        if (write_from_read(request->fd, request->message_length, request) != -1) {
            ret_val = 200;
            send_response(connfd, 200, 0);
        } else {
            ret_val = 500;
            send_response(connfd, 500, 0);
        }
        LOG("%s,/%s,%d,%d\n", request->method, request->uri, ret_val, request->request_id);
        fflush(logfile);
        flock(request->fd, LOCK_UN);
        close(request->fd);
        // close(fd);
    } else if (error == 2) {
        request->fd = open_file(request->uri, O_WRONLY, &error, O_CREAT, 0, 0);
        if (error == 0) {
            flock(request->fd, LOCK_EX);
            if (write_from_read(request->fd, request->message_length, request) != -1) {
                ret_val = 201;
                send_response(connfd, 201, 0);
            } else {
                ret_val = 500;
                send_response(connfd, 500, 0);
            }
            LOG("%s,/%s,%d,%d\n", request->method, request->uri, ret_val, request->request_id);
            fflush(logfile);
            flock(request->fd, LOCK_UN);
            close(request->fd);
            // close(fd);
        } else {
            ret_val = 500;
            send_response(connfd, 500, 0);
        }
    } else if (error == 13) {
        ret_val = 403;
        send_response(connfd, 403, 0);
    } else if (error == 20) {
        ret_val = 500;
        send_response(connfd, 500, 0);
    }
    return ret_val;
}

int process_append_request(Req_struct *request, int connfd, FILE *logfile) {
    int ret_val = 500;
    int error = 0;
    request->fd = open_file(request->uri, O_WRONLY, &error, 0, 0, O_APPEND);
    if (error == 0) {
        flock(request->fd, LOCK_EX);
        // Write to the file based on the message length.
        if (write_from_read(request->fd, request->message_length, request) != -1) {
            ret_val = 200;
            send_response(connfd, 200, 0);
        } else {
            ret_val = 500;
            send_response(connfd, 500, 0);
        }
        LOG("%s,/%s,%d,%d\n", request->method, request->uri, ret_val, request->request_id);
        fflush(logfile);
        flock(request->fd, LOCK_UN);
        close(request->fd);
        // close(fd);
    } else if (error == 2) {
        ret_val = 404;
        send_response(connfd, 404, 0);
    } else if (error == 13) {
        ret_val = 403;
        send_response(connfd, 403, 0);
    } else if (error == 20) {
        ret_val = 500;
        send_response(connfd, 500, 0);
    }
    return ret_val;
}

int process_request(Req_struct *request, int connfd, FILE *logfile) {
    int ret_val = 500;
    // Check to see if the request was valid
    if (request->valid == 0) {
        if (strcmp(request->method, GET_METHOD) == 0) {
            ret_val = process_get_request(request, connfd, logfile);
        } else if (strcmp(request->method, PUT_METHOD) == 0) {
            if (request->message_length) {
                ret_val = process_put_request(request, connfd, logfile);
            } else {
                ret_val = 400;
                send_response(connfd, 400, 0);
            }
        } else if (strcmp(request->method, APPEND_METHOD) == 0) {
            if (request->message_length) {
                ret_val = process_append_request(request, connfd, logfile);
            } else {
                ret_val = 400;
                send_response(connfd, 400, 0);
            }
        } else {
            ret_val = 501;
            send_response(connfd, 501, 0);
        }
    } else {
        ret_val = 400;
        send_response(connfd, 400, 0);
    }
    return ret_val;
}
