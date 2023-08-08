#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <regex.h>
#include <err.h>
#include <string.h>

#include "talk.h"
#include "parser.h"
#include "io.h"

#define LOG(...) fprintf(logfile, __VA_ARGS__);

int recv_request(Read_request *rq) {
    /**
	 * @brief This function is a simple wrapper around the system call function read. This wrapper is meant to listen
     * on the fd for a request and will continue call read untill it is able to match an http request. It will then store
     * the request into the buffer.
     * 
     * @param int infile_fd: The file descriptor to read from (this case will be a socket).
     * @param char *buf: A buffer to store the read bytes into.
     * @param int nbytes: The amount of bytes you want to read.
	 */

    int ret_val = 0;
    // Start off by trying to read REQUEST_LENGTH bytes as that is the max a packet can be.
    regex_t re;
    if (regcomp(&re, RECV_REGEX, REG_EXTENDED) != 0) {
        // Todo Error handling here for re compiling error
        ret_val = -1;
    } else {
        // While there are still valid bytes to read, keep reading.
        while (ret_val != -1 && rq->bytes_to_read) {
            // Try to read all the bytes left to be read and store in buf where we have not written yet.
            int curr_bytesread = read(
                rq->connfd, &(rq->request)[REQUEST_LENGTH - rq->bytes_to_read], rq->bytes_to_read);
            if (curr_bytesread == -1) {
                ret_val = -1;
                break;
            } else if (curr_bytesread > 0) {
                // We subtract the amount of bytes read from the total bytes left to read.
                rq->bytes_to_read -= curr_bytesread;
                ret_val += curr_bytesread;
                // See if we may have found the end of request sign "\r\n\r\n"
                if (regexec(&re, rq->request, (size_t) 0, NULL, 0) == 0) {
                    regex_t re_split;
                    regmatch_t re_split_i;
                    // Need to find the end of the "\r\n\r\n"
                    if (regcomp(&re_split, RECV_SPLITTER, REG_EXTENDED) == 0) {
                        if (regexec(&re_split, rq->request, 1, &re_split_i, 0) == 0) {
                            // HAve we processed more bytes then the end of the \r\n\r\n
                            if (re_split_i.rm_eo < ret_val) {
                                rq->message_start = re_split_i.rm_eo;
                            }
                        }
                    }
                    regfree(&re_split);
                    break;
                }
                // If we have ran into a null terminating string we need to leave, regex wont work.
                if ((int) strlen(rq->request) < ret_val) {
                    break;
                }
            } else {
                // If we have read end of file we break out of the loop.
                break;
            }
        }
        regfree(&re);
    }
    return ret_val;
}

void send_response(int connfd, int status_code, int response_content_length) {
    char status_header_line[REQUEST_LENGTH] = { 0 };
    // Determine the status line of the response.
    switch (status_code) {
    case 200:
        strcpy(status_header_line, "HTTP/1.1 200 OK\r\n");
        if (response_content_length) {
            // If we specified a content length then we use the one given and leave the message body for the caller to write
            sprintf(status_header_line + 17, "Content-Length: %d\r\n\r\n", response_content_length);
        } else {
            // Else use the default for OK
            strcpy(status_header_line + 17, "Content-Length: 3\r\n\r\nOK\n");
        }
        break;
    case 201:
        strcpy(status_header_line, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n");
        break;
    case 400:
        strcpy(status_header_line,
            "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        break;
    case 403:
        strcpy(
            status_header_line, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        break;
    case 404:
        strcpy(
            status_header_line, "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n");
        break;
    case 500:
        strcpy(status_header_line, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: "
                                   "22\r\n\r\nInternal Server Error\n");
        break;
    case 501:
        strcpy(status_header_line,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n");
        break;
    }
    write_bytes(connfd, status_header_line, strlen(status_header_line));
    return;
}

Read_request *create_read_request(int connfd) {
    Read_request *ret_val = (Read_request *) calloc(1, sizeof(Read_request));
    if (ret_val) {
        ret_val->connfd = connfd;
        ret_val->bytes_to_read = REQUEST_LENGTH;
        ret_val->request_length = 0;
        ret_val->message_start = 0;
    }
    return ret_val;
}

void destroy_read_request(Read_request **rq) {
    if (*rq) {
        free(*rq);
        *rq = NULL;
    }
    return;
}
