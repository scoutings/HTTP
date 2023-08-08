#include <stdlib.h>
#include <stdio.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "talk.h"
#include "parser.h"

#define BLOCK 4096

int open_file(
    char *path, int oflag, int *error_code, int create_flag, int trunc_flag, int append_flag) {
    int ret_val = 0;
    if (append_flag) {
        ret_val = open(path, oflag | append_flag);
    } else if (create_flag) {
        ret_val = open(path, oflag | create_flag, 0600);
    } else if (trunc_flag) {
        ret_val = open(path, oflag | trunc_flag);
    } else {
        ret_val = open(path, oflag);
    }
    // If there was a problem with opening theh file set the error code to errno.
    if (ret_val == -1) {
        *error_code = errno;
    } else {
        // Else set the error code to 0 to indicate that nothting went wrong.
        *error_code = 0;
    }
    return ret_val;
}

int read_bytes(int infile, char *buf, int nbytes, int *error_code) {
    int ret_val = 0;
    int bytes_left = nbytes;
    *error_code = 0;
    // While there are still soem bytes to read we keep reading
    while (bytes_left) {
        // We store the bytes read in the index that we have not written to yet.
        int temp_read = read(infile, &buf[nbytes - bytes_left], bytes_left);
        // If the infile has read some bytes
        if (temp_read > 0) {
            ret_val += temp_read;
            bytes_left -= temp_read;
        } else if (temp_read == -1) {
            // If there was an error return -1 and record the error.
            *error_code = errno;
            ret_val = -1;
            break;
        } else {
            break;
        }
    }
    return ret_val;
}

int write_bytes(int outfile, char *buf, int nbytes) {
    int ret_val = 0;
    int bytes_towrite = nbytes;
    // While there is still some bytes to write to the file
    while (bytes_towrite) {
        // Try to write all of the bytes left to the file from the correct spot in the buffer
        int curr_byteswritten = write(outfile, &buf[nbytes - bytes_towrite], bytes_towrite);
        // If there was an error return -1
        if (curr_byteswritten == -1) {
            ret_val = -1;
            break;
        } else if (curr_byteswritten > 0) {
            // Increment the amounfof bytes written and decrement bytes left
            ret_val += curr_byteswritten;
            bytes_towrite -= curr_byteswritten;
        } else {
            break;
        }
    }
    return ret_val;
}

// int write_from_read(int outfile, int connfd, int nbytes, Req_struct *request) {
int write_from_read(int outfile, int nbytes, Req_struct *request) {
    // char write_buffer[BLOCK] = { 0 };
    int ret_val = 0;
    // If we read some of the message body from the recv then we write it.
    if (request->curr_message_length) {
        if (request->curr_message_length > nbytes) {
            ret_val += write_bytes(outfile, request->message, nbytes);
        } else {
            ret_val += write_bytes(outfile, request->message, request->curr_message_length);
        }
    }
    // If we still have bytes to write in the message body write them.
    if (request->message_length - request->curr_message_length > 0) {
        ret_val += write_bytes(
            outfile, request->message_body, request->message_length - request->curr_message_length);
    }
    // while (ret_val < nbytes) {
    //     // Read As many bytes as you can and add it to the buffer.
    //     int bytes_read = read(connfd, write_buffer, BLOCK);
    //     ret_val += bytes_read;
    //     if (bytes_read > 0) {
    //         // If we have read enough or too many bytes
    //         if (ret_val >= nbytes) {
    //             // Write only the necissary bytes and break
    //             if (write_bytes(outfile, write_buffer, bytes_read - (ret_val - nbytes)) == -1) {
    //                 // If there was an error break
    //                 ret_val = -1;
    //                 break;
    //             }
    //             break;
    //         } else {
    //             if (write_bytes(outfile, write_buffer, bytes_read) == -1) {
    //                 // If there was an error break
    //                 ret_val = -1;
    //                 break;
    //             }
    //         }
    //         memset(write_buffer, 0, BLOCK * sizeof(char));
    //     } else if (bytes_read == 0) {
    //         break;
    //     } else {
    //         ret_val = -1;
    //         break;
    //     }
    // }
    return ret_val;
}
