#ifndef __IO_H__
#define __IO_H__

#include "parser.h"

int open_file(
    char *path, int oflag, int *error_code, int create_flag, int trunc_flag, int append_flag);

int read_bytes(int infile, char *buf, int nbytes, int *error_code);

int write_bytes(int outfile, char *buf, int nbytes);

int write_from_read(int outfile, int nbytes, Req_struct *request);

#endif
