/* request.c: Request structure */

#include "mq/request.h"

#include <stdlib.h>
#include <string.h>

/**
 * Create Request structure.
 * @param   method      Request method string.
 * @param   uri         Request uri string.
 * @param   body        Request body string.
 * @return  Newly allocated Request structure.
 */
Request * request_create(char *method, const char *uri, const char *body) {

    Request *r = calloc(1, sizeof(Request));

    if (!r){
        return NULL;
    }

    r->method = strdup(method);
    r->uri = strdup(uri);

    // allocate only if there, else set to null
    if(body) {
        r->body = strdup(body);
    } else {
        r->body = NULL;
    }

    r->next = NULL;
    
    return r;
}

/**
 * Delete Request structure.
 * @param   r           Request structure.
 */
void request_delete(Request *r) {
    free(r->method);
    free(r->uri);

    // free only if there
    if(r->body)
        free(r->body);

    free(r);
}

/**
 * Write HTTP Request to stream:
 *  
 *  $METHOD $URI HTTP/1.0\r\n
 *  Content-Length: Length($BODY)\r\n
 *  \r\n
 *  $BODY
 *      
 * @param   r           Request structure.
 * @param   fs          Socket file stream.
 */
void request_write(Request *r, FILE *fs) {
    fprintf(fs, "%s %s HTTP/1.0\r\n", r->method, r->uri);

    // only print body if there, else just print \r\n
    if(r->body) {
        fprintf(fs, "Content-Length: %lu\r\n", strlen(r->body));
        fprintf(fs, "\r\n");
        fprintf(fs, "%s", r->body);
    } else {
        fprintf(fs, "\r\n");
    }
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */ 
