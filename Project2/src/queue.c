/* queue.c: Concurrent Queue of Requests */

#include "mq/queue.h"

/**
 * Create queue structure.
 * @return  Newly allocated queue structure.
 */
Queue * queue_create() {
    Queue *q = calloc(1, sizeof(Queue));

    if (!q){
        return NULL;
    }

    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    sem_init(&q->semmy_cond, 0, 0);
    sem_init(&q->semmy_lock, 0, 1);

    return q;
}

/**
 * Delete queue structure.
 * @param   q       Queue structure.
 */
void queue_delete(Queue *q) {

    Request *curr = q->head;
    while (curr != NULL){
        Request *next = curr->next;;
        request_delete(curr);
        curr = next;
    }

    free(q);
}

/**
 * Push request to the back of queue.
 * @param   q       Queue structure.
 * @param   r       Request structure.
 */
void queue_push(Queue *q, Request *r) {

    sem_wait(&q->semmy_lock);

    if (q->head == NULL){
        q->head = r;
        q->tail = r;
    } else{
        q->tail->next = r;
        q->tail = r;
    }

    q->size += 1;
    sem_post(&q->semmy_lock);

    sem_post(&q->semmy_cond);
}

/**
 * Pop request to the front of queue (block until there is something to return).
 * @param   q       Queue structure.
 * @return  Request structure.
 */
Request * queue_pop(Queue *q) {

    sem_wait(&q->semmy_cond);
    sem_wait(&q->semmy_lock);

    Request *popped = q->head;
    q->head = q->head->next;

    q->size -= 1;
    sem_post(&q->semmy_lock);

    return popped;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
