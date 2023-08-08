#ifndef __PARSER_H__
#define __PARSER_H__

#include "talk.h"

typedef struct Req_struct Req_struct;

struct Req_struct {
    int valid;
    char method[REQUEST_LENGTH];
    char uri[REQUEST_LENGTH];
    int message_length;
    int request_id;
    int curr_message_length;
    char *message;
    char *message_body;
    int fd;
};

void destroy_req(Req_struct **req);

void print_request(Req_struct *request);

Req_struct *parse_request(char *request, int request_length, int message_start, int fd);

#endif
