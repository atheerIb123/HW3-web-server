#include "segel.h"
#include "request.h"
#include "Queue.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

//For synchronization purposes
pthread_mutex_t m_queue;
pthread_mutex_t m_queue_is_full;
pthread_cond_t working_not_empty;
pthread_cond_t queue_not_full;

Queue* pending_requests_queue;
Queue* assigned_requests_queue;

// HW3: Parse the new arguments too
void getargs(int *port, int* threadsCount, int* queueSize, char* schedalg, int* maxSize, int argc, char* argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port> <threads_count> <queue_size>\n", argv[0]);
	exit(1);
    }
    
   
    *port = atoi(argv[1]);
    *threadsCount = atoi(argv[2]);
    *queueSize = atoi(argv[3]);
    strcpy(schedalg, argv[4]);
    
    if(strcmp(schedalg, "dynamic") == 0)
    {
		if (argc > 5)
		{
			*maxSize = atoi(argv[5]);
		}
	}
	
	if (argc < 10) {
		fprintf(stderr, "Usage: %s <port> <threads_count> <queue_size>\n", schedalg);
	}
}

void* worker_request_handler(void* args)
{
    int threadInitId = *((int*)args);
    free(args);
    
    struct timeval entryTime;
    Stats st = malloc(sizeof(*st));

    st->workingThreadId = threadInitId;
    st->requestCount = 0;
    st->workingThreadsStaticCount = 0;
    st->workingThreadsDynamicCount = 0;


    while(1)
    {
        pthread_mutex_lock(&m_queue);

        while(pending_requests_queue->size == 0)
        {
            pthread_cond_wait(&working_not_empty, &m_queue);
        }

        Request* requestToHandle = getFirstRequest(pending_requests_queue);//getFirst(waiting_req_queue);

        //Update stats
        gettimeofday(&entryTime, NULL);
        timersub(&entryTime, &requestToHandle->arrive, &requestToHandle->dispatch);
        st->arrivalTime = requestToHandle->arrive;
        st->dispachInterval = requestToHandle->dispatch;

        Dequeue(pending_requests_queue);

        node* added_request_node = addToQueue(assigned_requests_queue, requestToHandle);
        pthread_mutex_unlock(&m_queue);

        requestHandle(requestToHandle->confd, st);

        Close(requestToHandle->confd);
        free(requestToHandle);

        //Now we need to dequeue the handled request from the requests_being_handled queue
        pthread_mutex_lock(&m_queue);
        removeFromQueue(assigned_requests_queue, added_request_node);
        pthread_cond_signal(&queue_not_full);
        pthread_mutex_unlock(&m_queue);
    }

    free(st);
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threadsCount, queueSize, maxSize;
    char schedalg[7];
    struct timeval arrivalTime;
    struct sockaddr_in clientaddr;

    getargs(&port, &threadsCount, &queueSize, schedalg, &maxSize, argc, argv);

    //Creating them threads
    pthread_t* threadsArray = (pthread_t*)malloc(sizeof(pthread_t) * threadsCount);
	
	
    for(int id = 0; id < threadsCount ; id++)
    {
		if(threadsCount > 0)
		{
			fprintf(stderr, "MY GOD %d\n", threadsCount); 
		}
		int* id_to_pass = malloc(sizeof(int));
		*id_to_pass = id;
        pthread_create(&threadsArray[id], NULL, worker_request_handler, id_to_pass);
    }
	
    //Initializing locks + conds
    pthread_mutex_init(&m_queue, NULL);
    pthread_mutex_init(&m_queue_is_full, NULL);
    pthread_cond_init(&working_not_empty, NULL);
    pthread_cond_init(&queue_not_full, NULL);

    //Initializing the request handling queues
    pending_requests_queue = createQueue();
    assigned_requests_queue = createQueue();
	if(threadsCount > 0)
	{
		fprintf(stderr, "MY GOD %d\n", threadsCount); 
	}
    listenfd = Open_listenfd(port);
	if(threadsCount > 0)
	{
		fprintf(stderr, "MY GOD %d\n", threadsCount); 
	}
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

        //In case the schedalg is random/drop head
        gettimeofday(&arrivalTime, NULL);
        Request* newRequest = initRequest(connfd, arrivalTime);
        newRequest->confd = connfd;

        if (pending_requests_queue->size + assigned_requests_queue->size >= queueSize)
        {
            if(strcmp(schedalg, "block") == 0)
            {
                pthread_mutex_lock(&m_queue_is_full);

                while(pending_requests_queue->size + assigned_requests_queue->size >= queueSize)
                {
                    pthread_cond_wait(&queue_not_full, &m_queue_is_full);
                }

                pthread_mutex_unlock(&m_queue_is_full);
            }
            else if(strcmp(schedalg, "dt") == 0)
            {
                Close(connfd);
            }
            else if(strcmp(schedalg, "dh") == 0)
            {
                if(pending_requests_queue->size > 0)
                {
                    pthread_mutex_lock(&m_queue);
                    Close(pending_requests_queue->head->data->confd);
                    free(pending_requests_queue->head->data);
                    removeFromQueue(pending_requests_queue, pending_requests_queue->head);

                    addToQueue(pending_requests_queue, newRequest);
                    //pthread_cond_signal(&working_not_empty);
                    pthread_mutex_unlock(&m_queue);
                }
                else //queue size = 0 (an empty queue)
                {
                    Close(connfd);
                }
            }
            else if(strcmp(schedalg, "bf") == 0)
            {
                pthread_mutex_lock(&m_queue_is_full);

                while(pending_requests_queue->size > 0 && assigned_requests_queue->size > 0)
                {
                    pthread_cond_wait(&working_not_empty, &m_queue_is_full);
                }

                pthread_mutex_unlock(&m_queue_is_full);
                Close(connfd);
            }
            else if(strcmp(schedalg, "dynamic") == 0)
            {
                Close(connfd);
                queueSize++;
                
                if (maxSize < queueSize)
                {
					maxSize = queueSize;
				}
                
            }
            else if(strcmp(schedalg, "random") == 0)
            {
                if(pending_requests_queue->size == 0)
                {
                    Close(connfd);
                }
                else
                {
                    pthread_mutex_lock(&m_queue);

                    int amountToDrop = (pending_requests_queue->size + 1) / 2;

                    for(int i = 0; i < amountToDrop; i++)
                    {
                        int randIndex = rand() % pending_requests_queue->size;
                        removeByIndex(pending_requests_queue, randIndex);
                    }

                    addToQueue(pending_requests_queue, newRequest);
                    pthread_cond_signal(&working_not_empty);
                    pthread_mutex_unlock(&m_queue);
                }
            }
        }

        Close(connfd);
    }

    destroyAndCloseDFds(assigned_requests_queue);
    destroy(pending_requests_queue);

    for(int i = 0 ; i < threadsCount ; i++)
    {
        pthread_join(threadsArray[i], NULL);
    }

    free(threadsArray);
}


    


 
