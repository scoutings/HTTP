#ifndef __TALK_H__
#define __TALK_H__

#define REQUEST_LENGTH 2048
#define RECV_REGEX     ".+\r\n\r\n.*"
#define RECV_SPLITTER  "\r\n\r\n"

typedef struct Read_request Read_request;

struct Read_request {
    int connfd;
    char request[REQUEST_LENGTH + 1];
    int bytes_to_read;
    int request_length;
    int message_start;
};

int recv_request(Read_request *rq);

void send_response(int connfd, int error_code, int content_length);

Read_request *create_read_request(int connfd);

void destroy_read_request(Read_request **rq);

#endif
