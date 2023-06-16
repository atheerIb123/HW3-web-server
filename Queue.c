#include "Queue.h"

Queue* createQueue()
{
    Queue* q = malloc(sizeof(q));
    q->size = 0;
    q->head = NULL;
    q->tail = NULL;
    return q;
}

node* getHead(Queue* q)
{
    return q->head;
}

Request* getFirstRequest(Queue* q)
{
    return q->head->data;
}

void Dequeue(Queue* q)
{
    if(q == NULL)
        return;
    if(q->size != 0)
        removeFromQueue(q,q->head);
}

node* addToQueue(Queue* q , Request* data)
{ //last element is always null so add it before the null
    node* tempTail = q->tail;
    node* newNode = malloc(sizeof (newNode));
    newNode->data = data;
    newNode->previous = NULL;
    q->tail = newNode;
    if(q->size == 0)
    {
        q->head = newNode;
        newNode->next = NULL;
        q->size++;
        return newNode;
    }
    newNode->next = tempTail;
    tempTail->previous = newNode;
    q->size++;
    return newNode;
}

void printQueue(Queue* q)
{
    node* current = q->head;
    while(current != NULL)
    {
        printf("%d", current->data->confd);
        if(current != q->head && current->next != NULL)
        {
            printf(" -> ");
        }
        current = current->next;
    }
    printf("\n");
}

void removeFromQueue(Queue* q, node* nodeToDelete)
{
    if(q == NULL || nodeToDelete == NULL)
        return;
    if(nodeToDelete->previous == NULL && nodeToDelete->next == NULL)
    {
        q->tail = NULL;
        q->head = NULL;
    }
    else if(nodeToDelete->previous == NULL)
    {
        nodeToDelete->previous->next = NULL;
        q->head = nodeToDelete->previous;
    }
    else if(nodeToDelete->next == NULL)
    {
        nodeToDelete->next->previous = NULL;
        q->tail = nodeToDelete->next;
    }
    else
    {
        nodeToDelete->previous->next = nodeToDelete->next;
        nodeToDelete->next->previous = nodeToDelete->previous;
    }
    free(nodeToDelete);
    q->size--;
}

void removeByIndex(Queue* q, int index)
{
    if(q == NULL || index > q->size || index < 0)
        return;
    node* current = q->tail;
    for(int i = 0; i < q->size; i++)
    {
        if(i == index)
        {
            Close(current->data->confd);
            free(current->data);
            removeFromQueue(q,current);
            return;
        }
        current = current->next;
    }
}

void removeIndices(Queue* q, int* indicesToDelete)
{
    if(q == NULL)
        return;
    int currentIndex = q->size - 1;
    node* current = q->head;
    while(current != NULL)
    {
        if(indicesToDelete[currentIndex] == 1)
        {
            Close(current->data->confd);
            free(current->data);
            removeFromQueue(q, current);
        }
        currentIndex--;
        current = current->previous;
    }
}

void destroy(Queue* q)
{
    if(q == NULL)
        return;
    while(q->size > 0)
    { //dequeue if not empty
        Dequeue(q);
    }
    free(q);
}

void queueDestroyAndClose(Queue* q)
{
    if(q == NULL)
        return;
    while(q->size > 0)
    {
        Close(q->head->data->confd);
        free(q->head->data);
        Dequeue(q);
    }
    free(q);
}

Request* initRequest(int conFd, struct timeval arrivalTime)
{
    Request* newRequest = (Request*)malloc(sizeof(Request));
    newRequest->confd = conFd;
    newRequest->arrive= arrivalTime;

    return newRequest;
}
