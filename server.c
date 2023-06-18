#include <stdbool.h>
#include "segel.h"
#include "request.h"
#include "Queue.h"

void getargs(int *port, int *threads_num, int* queue_size, char schedAlg[7], int* maxSize, int argc, char *argv[])
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <port> <threads_num> <queue_size> <schedalg>\n", argv[0]);
        exit(1);
    }

    *port = atoi(argv[1]);
    *threads_num = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    strcpy(schedAlg, argv[4]);

    if(strcmp(argv[4], "dynamic") == 0)
    {
        if(argc >= 6)
        {
            *maxSize = atoi(argv[5]);
        }
    }
}

Queue* q_waiting_req;
Queue* q_handling_req;

pthread_cond_t c_wr_not_empty;
pthread_mutex_t m_queue;
pthread_cond_t c_queue_not_full;
pthread_mutex_t m_queue_full;

void* workerFunc(void* arg) {
    int id = *(int*)arg;
    free(arg);
    Stats stats = malloc(sizeof (*stats));

    stats->handler_thread_id = id;
    stats->handler_thread_req_count = 0;
    stats->handler_thread_static_req_count = 0;
    stats->handler_thread_dynamic_req_count = 0;

    struct timeval picked;
    while (1)
    {
        pthread_mutex_lock(&m_queue);
        while (q_waiting_req->size == 0)
        {
            pthread_cond_wait(&c_wr_not_empty, &m_queue);
        }

        Request* request_to_handle = getFirstRequest(q_waiting_req); // last in list = first in queue

        Dequeue(q_waiting_req);

        gettimeofday(&picked,NULL);
        timersub(&picked, &request_to_handle->arrive, &request_to_handle->dispatch);
        stats->dispatch_interval = request_to_handle->dispatch;
        stats->arrival_time = request_to_handle->arrive;


        node* req_Node = addToQueue(q_handling_req, request_to_handle);

        pthread_mutex_unlock(&m_queue);

        struct timeval timeStartedHandling;
        gettimeofday(&timeStartedHandling,NULL);

        requestHandle(request_to_handle->confd,stats);

        struct timeval timeDoneHandling;
        gettimeofday(&timeDoneHandling,NULL);
        struct timeval timeTook;
        timersub(&timeDoneHandling, &timeStartedHandling, &timeTook);

        Close(request_to_handle->confd);
        free(request_to_handle);

        pthread_mutex_lock(&m_queue);
        removeFromQueue(q_handling_req, req_Node);

        pthread_cond_signal(&c_queue_not_full);
        pthread_mutex_unlock(&m_queue);
    }
    free(stats);
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, threads_num, queue_size, clientlen, maxSize;
    bool addRequestPolicy;
    char schedalg[8];
    struct sockaddr_in clientaddr;
    struct timeval arrive;

    pthread_mutex_init(&m_queue, NULL);
    pthread_cond_init(&c_wr_not_empty, NULL);
    pthread_mutex_init(&m_queue_full, NULL);
    pthread_cond_init(&c_queue_not_full, NULL);

    getargs(&port, &threads_num, &queue_size, schedalg, &maxSize, argc, argv);

    q_waiting_req = createQueue();
    q_handling_req = createQueue();

    pthread_t* thread_list = (pthread_t*) malloc(threads_num * sizeof(pthread_t));

    for (int i = 0; i < (int) threads_num; i++)
    {
        int* id_to_pass = malloc(sizeof(int));
        *id_to_pass = i;
        pthread_create(&thread_list[i], NULL,  workerFunc, id_to_pass);
    }

    listenfd = Open_listenfd(port);

    while (1)
    {
        addRequestPolicy = false;
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        gettimeofday(&arrive,NULL);

        if (q_waiting_req->size + q_handling_req->size >= queue_size) {

            if(strcmp(schedalg,"block") == 0)
            {
                pthread_mutex_lock(&m_queue_full);

                while (q_waiting_req->size + q_handling_req->size >= queue_size)
                {
                    pthread_cond_wait(&c_queue_not_full, &m_queue_full);
                }

                pthread_mutex_unlock(&m_queue_full);
            }
            else if (strcmp(schedalg,"dh") == 0)
            {
                if(q_waiting_req->size != 0)
                {
                    pthread_mutex_lock(&m_queue);
                    Close(q_waiting_req->head->data->confd);
                    free(q_waiting_req->head->data);
                    Dequeue(q_waiting_req);
                    pthread_mutex_unlock(&m_queue);
                }
                else
                {
                    Close(connfd);
                    addRequestPolicy = true;
                }
            }
            else if (strcmp(schedalg, "dt") == 0)
            {
                Close(connfd);
                addRequestPolicy = true;
            }
            else if(strcmp(schedalg, "bf") == 0)
            {
                pthread_mutex_lock(&m_queue_full);

                while(q_waiting_req->size > 0 && q_handling_req->size > 0)
                {
                    pthread_cond_wait(&c_wr_not_empty, &m_queue_full);
                }

                pthread_mutex_unlock(&m_queue_full);
                Close(connfd);
            }
            else if(strcmp(schedalg, "dynamic") == 0)
            {
                queue_size++;

                if (queue_size == maxSize) //drop tail policy
                {
                    Close(connfd);
                    addRequestPolicy = true;
                }
            }
            else if (strcmp(schedalg, "random") == 0)
            {
                if(q_handling_req->size >= queue_size)
                {
                    Close(connfd);
                    addRequestPolicy = true;
                }
                else
                {
                    pthread_mutex_lock(&m_queue);

                    int amountToDrop = (q_waiting_req->size + 1) / 2;

                    for(int i = 0; i < amountToDrop; i++)
                    {
                        int randIndex = rand() % q_waiting_req->size;
                        removeByIndex(q_waiting_req, randIndex);
                    }

                    pthread_mutex_unlock(&m_queue);
                }
            }

        }

        if (addRequestPolicy != true)
        {
            Request* request = malloc(sizeof(*request));
            request->confd = connfd;
            request->arrive=arrive;
            pthread_mutex_lock(&m_queue);

            addToQueue(q_waiting_req, request);

            pthread_cond_signal(&c_wr_not_empty);
            pthread_mutex_unlock(&m_queue);
        }

    }

    destroyAndCloseDFds(q_handling_req);
    destroy(q_waiting_req);

    for (int i = 0; i < threads_num; i++) {
        pthread_join(thread_list[i], NULL);
    }

    free(thread_list);
}


 
