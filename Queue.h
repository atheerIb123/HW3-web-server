#ifndef Queue_H
#define Queue_H

#include "segel.h"

struct Request
{
    int confd;
    struct timeval arrive;
    struct timeval dispatch;
};
typedef struct Request Request;

struct node {

    Request* data;
    struct node* next;
    struct node* previous;
};
typedef struct node node;

struct queue
{
    int size;
    struct node *head;
    struct node *tail;
};

typedef struct queue Queue;

Queue* createQueue();
node* getHead(Queue* q);
Request* getFirstRequest(Queue* q);
void Dequeue(Queue* q);
node* addToQueue(Queue* q , Request* data);
void printQueue(Queue* q);
void removeFromQueue(Queue* q, node* nodeToDelete);
void removeByIndex(Queue* q, int index);
void removeIndices(Queue* q, int* indicesToDelete);
void destroy(Queue* q);
void destroyAndCloseDFds(Queue* q);

#endif //Queue_H