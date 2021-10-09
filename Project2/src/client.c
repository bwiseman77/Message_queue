/* client.c: Message Queue Client */

#include "mq/client.h"
#include "mq/logging.h"
#include "mq/socket.h"
#include "mq/string.h"

/* Internal Constants */

#define SENTINEL "SHUTDOWN"

/* Internal Prototypes */

void * mq_pusher(void *);
void * mq_puller(void *);

/* External Functions */

/**
 * Create Message Queue withs specified name, host, and port.
 * @param   name        Name of client's queue.
 * @param   host        Address of server.
 * @param   port        Port of server.
 * @return  Newly allocated Message Queue structure.
 */
MessageQueue * mq_create(const char *name, const char *host, const char *port) {

    MessageQueue *mq = calloc(1, sizeof(MessageQueue));

    strcpy(mq->name, name);
    strcpy(mq->host, host);
    strcpy(mq->port, port);    

    mq->outgoing = queue_create();
    mq->incoming = queue_create();
    mq->shutdown = false;
    
    if(sem_init(&mq->semmy, 0, 1) < 0)
        return NULL;
    
    
    return mq;
}

/**
 * Delete Message Queue structure (and internal resources).
 * @param   mq      Message Queue structure.
 */
void mq_delete(MessageQueue *mq) {

    queue_delete(mq->outgoing);
    queue_delete(mq->incoming);

    free(mq); 
}

/**
 * Publish one message to topic (by placing new Request in outgoing queue).
 * @param   mq      Message Queue structure.
 * @param   topic   Topic to publish to.
 * @param   body    Message body to publish.
 */
void mq_publish(MessageQueue *mq, const char *topic, const char *body) {

    /* load uri */
    char uri[BUFSIZ];
    sprintf(uri, "/topic/%s", topic);

    /* create request and add to outgoing */
    Request *r = request_create("PUT", uri, body);


    queue_push(mq->outgoing, r);

    /* signal that we might have added a SENTINEL */
    //sem_post(&mq->semmy);
}

/**
 * Retrieve one message (by taking Request from incoming queue).
 * @param   mq      Message Queue structure.
 * @return  Newly allocated message body (must be freed).
 */
char * mq_retrieve(MessageQueue *mq) {

    /* pop from incoming */
    Request *r = queue_pop(mq->incoming);
    if(streq(r->body, SENTINEL)) {
        request_delete(r);
        return NULL;
    }

    /* copy to send back */
    char* giveback = strdup(r->body);

    /* delete request */
    request_delete(r);

    return giveback;
}

/**
 * Subscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to subscribe to.
 **/
void mq_subscribe(MessageQueue *mq, const char *topic) {

    /* load uri */
    char uri[BUFSIZ];
    sprintf(uri, "/subscription/%s/%s", mq->name, topic);

    /* create request and add to outgoing queue */
    Request *r = request_create("PUT", uri, NULL);

    queue_push(mq->outgoing, r);

}

/**
 * Unubscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to unsubscribe from.
 **/
void mq_unsubscribe(MessageQueue *mq, const char *topic) {

    /* load uri */
    char uri[BUFSIZ];
    sprintf(uri, "/subscription/%s/%s", mq->name, topic);

    /* create request and add to outgoing queue */
    Request *r = request_create("DELETE", uri, NULL);
    queue_push(mq->outgoing, r);
}

/**
 * Start running the background threads:
 *  1. First thread should continuously send requests from outgoing queue.
 *  2. Second thread should continuously receive reqeusts to incoming queue.
 * @param   mq      Message Queue structure.
 */
void mq_start(MessageQueue *mq) {

    /* create pusher and puller threads, then subscribe to SENTINEL */
    // built in error checking //
    thread_create(&mq->pusher, NULL, mq_pusher, (void *)mq);
    thread_create(&mq->puller, NULL, mq_puller, (void *)mq);

    mq_subscribe(mq, SENTINEL);
}

/**
 * Stop the message queue client by setting shutdown attribute and sending
 * sentinel messages
 * @param   mq      Message Queue structure.
 */
void mq_stop(MessageQueue *mq) {

    // publish sentinel
    mq_publish(mq, SENTINEL, SENTINEL);

    // set shutdown bool to true
    sem_wait(&mq->semmy);
    mq->shutdown = true;
    sem_post(&mq->semmy);

    // join threads
    // built in error checking //
    thread_join(mq->pusher, NULL);
    thread_join(mq->puller, NULL);
}

/**
 * Returns whether or not the message queue should be shutdown.
 * @param   mq      Message Queue structure.
 */
bool mq_shutdown(MessageQueue *mq) {
    
    /* wait till after something publishes before checking shutdown.    */
    /* Could be getting lucky?                                          */
    /* Ask Bui                                                          */

    sem_wait(&mq->semmy);
    bool temp = mq->shutdown;
    sem_post(&mq->semmy);

    return temp;
}

/* Internal Functions */

/**
 * Pusher thread takes messages from outgoing queue and sends them to server.
 **/
void * mq_pusher(void *arg) {

    MessageQueue *mq = (MessageQueue  *)arg;

    /* run thread till shutdown */
    while(!mq_shutdown(mq)) {

        /* pop request */
        Request *r = queue_pop(mq->outgoing);

        /* open connection */
        FILE *fs = socket_connect(mq->host, mq->port);
        if(!fs)
            continue;

        /* write request and free after */
        request_write(r, fs);
        request_delete(r);

        /* close file stream */
        fclose(fs);

    }

    return NULL;
}

/**
 * Puller thread requests new messages from server and then puts them in
 * incoming queue.
 **/
void * mq_puller(void *arg) {

    MessageQueue *mq = (MessageQueue  *)arg;

    /* run thread till shutdown */
    while(!mq_shutdown(mq)) {

        /* connect to host and port */
        FILE *fs = socket_connect(mq->host, mq->port);
        if(!fs) {
            fclose(fs);
            continue;
        }

        /* send request to server */
        char uri[BUFSIZ];
        sprintf(uri, "/queue/%s", mq->name);
        Request *r1 = request_create("GET", uri, NULL);

        request_write(r1, fs);
        request_delete(r1);

        /* read response status*/
        char buffer[BUFSIZ];
        if(!fgets(buffer, BUFSIZ, fs) || !strstr(buffer, "200 OK")){
            fclose(fs);
            continue;
        }

        /* read headers */
        size_t length = 0;
        while(fgets(buffer, BUFSIZ, fs) && strcmp(buffer, "\r\n")){
            sscanf(buffer, "Content-Length: %lu", &length);
        }

        /* read body */
        char body[BUFSIZ];
        fgets(body, BUFSIZ, fs);
                
        /* make request for queue */
        Request *r2 = request_create("GET", uri, body);

        queue_push(mq->incoming, r2);

        /* close file stream */
        fclose(fs);
    }

    return NULL;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
