#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "parser.h"
#include "talk.h"

#define VALID_REQUEST_LINE   "^[a-zA-Z]+ /[a-zA-Z0-9._/]+ HTTP/1.1$"
#define VALID_HEADER_LINE    "^[ -~]+: [ -~]+$"
#define VALID_HEADER_CL_LINE "Content-Length:"
#define VALID_HEADER_RI_LINE "Request-Id:"
#define VALID_HEADER_CL_VAL  "[0-9]+"
#define VALID_METHOD         "[a-zA-Z]+"
#define VALID_URI            "/[a-zA-Z0-9._/]+"
#define HTTP_DELIM           "\r\n"

static int last_index = 0;

Req_struct *create_req() {
    Req_struct *ret_val = (Req_struct *) calloc(1, sizeof(Req_struct));
    if (!ret_val) {
        ret_val = NULL;
    } else {
        // Assume a request is not valid and validate it when parsed.
        ret_val->valid = -1;
        ret_val->message_length = 0;
        ret_val->request_id = 0;
        ret_val->curr_message_length = 0;
        ret_val->message = NULL;
        ret_val->message_body = NULL;
        ret_val->fd = -1;
    }
    return ret_val;
}

void destroy_req(Req_struct **req) {
    if (*req) {
        // If the message exists free it
        if ((*req)->message) {
            free((*req)->message);
        }
        if ((*req)->message_body) {
            free((*req)->message_body);
        }
        free(*req);
        *req = NULL;
    }
    return;
}

void print_request(Req_struct *request) {
    if (request) {
        // Print the request
        printf("Valid: [%d]\nMethod: [%s]\nURI: [%s]\nMessage-Length: [%d]\nRequest-Id: "
               "[%d]\nMessage-body: [%s]\n",
            request->valid, request->method, request->uri, request->message_length,
            request->request_id, request->message);
    } else {
        printf("Error");
    }
    return;
}

int get_next_seperate(char *request, char *next_line, regex_t *delim) {
    int ret_val = -1;
    regmatch_t sep;
    // call till it gets to the one you want
    if (regexec(delim, request + last_index, 1, &sep, 0) == 0) {
        memcpy(next_line, &request[last_index], sep.rm_so);
        last_index = last_index + sep.rm_eo;
        ret_val = 0;
    }
    return ret_val;
}

int validate_parse_request_line(char *request, char *method, char *uri) {
    int ret_val = -1;
    regex_t re_requestline, re_method, re_uri;
    regmatch_t method_i, uri_i;
    // Compile all need regexes to valid the request line.
    if (regcomp(&re_requestline, VALID_REQUEST_LINE, REG_EXTENDED) == 0
        && regcomp(&re_method, VALID_METHOD, REG_EXTENDED) == 0
        && regcomp(&re_uri, VALID_URI, REG_EXTENDED) == 0) {
        // Check if the whole line matches the correct format
        if (regexec(&re_requestline, request, (size_t) 0, NULL, 0) == 0) {
            // Now that we know it is in the right format, we must find the method and the uri
            if (regexec(&re_method, request, 1, &method_i, 0) == 0
                && regexec(&re_uri, request, 1, &uri_i, 0) == 0) {
                // Copy there values to the given strings
                memcpy(method, &request[method_i.rm_so], method_i.rm_eo - method_i.rm_so);
                // Chop off the "/" as it is not needed internally.
                memcpy(uri, &request[uri_i.rm_so + 1], uri_i.rm_eo - uri_i.rm_so - 1);
                ret_val = 0;
            }
        }
        regfree(&re_requestline);
        regfree(&re_method);
        regfree(&re_uri);
    }
    return ret_val;
}

int validate_parse_header_field(char *request, int *cl_val, int *ri_val, regex_t *next_delim) {
    // Assume field is invalid
    int ret_val = -1;
    regex_t re_header, re_header_cl_line, re_header_ri_line;
    if (regcomp(&re_header, VALID_HEADER_LINE, REG_EXTENDED) == 0
        && regcomp(&re_header_cl_line, VALID_HEADER_CL_LINE, REG_EXTENDED) == 0
        && regcomp(&re_header_ri_line, VALID_HEADER_RI_LINE, REG_EXTENDED) == 0) {
        // Parse each header line
        while (1) {
            // Get the next line
            char next_line[REQUEST_LENGTH] = { 0 };
            get_next_seperate(request, next_line, next_delim);
            // If the next line is empty, we have not reached an error and return valid.
            if (*next_line == '\0') {
                // If we have gotten to the end with out finding an error then it is valid.
                ret_val = 0;
                break;
            } else {
                // Match on a valid header entry
                if (regexec(&re_header, next_line, (size_t) 0, NULL, 0) == 0) {
                    // Check to see if this is a valid Content Length entry.
                    if (regexec(&re_header_cl_line, next_line, (size_t) 0, NULL, 0) == 0) {
                        // Find get the content length value
                        regex_t re_header_cl_val;
                        regmatch_t header_cl_val;
                        // Compile the regex to grab the value.
                        if (regcomp(&re_header_cl_val, VALID_HEADER_CL_VAL, REG_EXTENDED) == 0) {
                            // Match the value.
                            char cl_value[REQUEST_LENGTH] = { 0 };
                            if (regexec(&re_header_cl_val, next_line, 1, &header_cl_val, 0) == 0) {
                                // Copy the value into the string.
                                memcpy(cl_value, &next_line[header_cl_val.rm_so],
                                    header_cl_val.rm_eo - header_cl_val.rm_so);
                                *cl_val = atoi(cl_value);
                            }
                            regfree(&re_header_cl_val);
                        } else {
                            break;
                        }
                        // Check to see if this is a valid Request-Id.
                    } else if (regexec(&re_header_ri_line, next_line, (size_t) 0, NULL, 0) == 0) {
                        // Get the Request ID
                        regex_t re_header_ri_val;
                        regmatch_t header_ri_val;
                        // Compile the regex to grab the value.
                        if (regcomp(&re_header_ri_val, VALID_HEADER_CL_VAL, REG_EXTENDED) == 0) {
                            // Match the value into the string.
                            char ri_value[REQUEST_LENGTH] = { 0 };
                            if (regexec(&re_header_ri_val, next_line, 1, &header_ri_val, 0) == 0) {
                                // Copy the value into the string.
                                memcpy(ri_value, &next_line[header_ri_val.rm_so],
                                    header_ri_val.rm_eo - header_ri_val.rm_so);
                                *ri_val = atoi(ri_value);
                            }
                            regfree(&re_header_ri_val);
                        } else {
                            break;
                        }
                    }
                } else {
                    // If we did not find one then we must break and return invalid.
                    break;
                }
            }
        }
        regfree(&re_header);
        regfree(&re_header_cl_line);
        regfree(&re_header_ri_line);
    }
    return ret_val;
}

Req_struct *parse_request(char *request, int request_length, int message_start, int fd) {
    // Create a request structure to fill out.
    Req_struct *ret_val = create_req();
    // If we still have a message in the header then copy it over
    if (ret_val) {
        if (message_start) {
            ret_val->message = (char *) calloc(1, sizeof(char) * REQUEST_LENGTH);
            ret_val->curr_message_length = request_length - message_start;
            memcpy(ret_val->message, &request[message_start], ret_val->curr_message_length);
        }
        regex_t re_next_delim;
        if (regcomp(&re_next_delim, HTTP_DELIM, REG_EXTENDED) == 0) {
            char request_line[REQUEST_LENGTH] = { 0 };
            last_index = 0;
            get_next_seperate(request, request_line, &re_next_delim);
            // Check the request line
            if (validate_parse_request_line(request_line, ret_val->method, ret_val->uri) == 0) {
                // Lastly check the Header field and if that is valid then the entire request is aswell.
                ret_val->valid = validate_parse_header_field(
                    request, &ret_val->message_length, &ret_val->request_id, &re_next_delim);
            }
            regfree(&re_next_delim);
        }
        // Now we must get the whole message body if there is one
        if (!(ret_val->valid) && ret_val->message_length > ret_val->curr_message_length) {
            // Figure out how many bytes are left to read
            int total_bytes = ret_val->message_length - ret_val->curr_message_length;
            // If any then read them
            int bytes_left = total_bytes;
            if (bytes_left) {
                ret_val->message_body = (char *) calloc(total_bytes + 1, sizeof(char));
                // While there are still bytes left to read
                while (bytes_left) {
                    int curr_read
                        = read(fd, &(ret_val->message_body)[total_bytes - bytes_left], bytes_left);
                    if (curr_read > 0) {
                        bytes_left -= curr_read;
                    } else {
                        break;
                    }
                }
            }
        }
    }
    // Return the request struct
    return ret_val;
}
