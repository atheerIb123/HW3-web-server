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

    int is_first = 0;
    int is_last = 0;
    if(nodeToDelete->previous == NULL)
        is_last = 1;
    if(nodeToDelete->next == NULL)
        is_first = 1;

    if(!is_first && !is_last) // not last and not first
    {
        nodeToDelete->next->previous=nodeToDelete->previous;
        nodeToDelete->previous->next=nodeToDelete->next;
    }
    else if(!is_last) // first and not last
    {
        nodeToDelete->previous->next=NULL;
        q->head = nodeToDelete->previous;

    }
    else if(!is_first) // last and not first
    {
        nodeToDelete->next->previous=NULL;
        q->tail = nodeToDelete->next;
    }
    else // first and last
    {
        q->tail=NULL;
        q->head=NULL;
    }
    free(nodeToDelete);
    q->size--;
}

void removeByIndex(Queue* q, int index_to_remove)
{
    if(q == NULL)
        return;
    node* node_to_remove = q->tail;
    int current_index = 0;

    while(current_index <= index_to_remove && node_to_remove != NULL)
    {
        if(current_index == index_to_remove)
        {
            Close(node_to_remove->data->confd);
            free(node_to_remove->data);
            removeFromQueue(q, node_to_remove);
            return;
        }
        node_to_remove = node_to_remove->next;
        current_index++;
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