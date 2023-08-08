#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "parser.h"

int process_request(Req_struct *request, int connfd, FILE *logfile);

#endif
