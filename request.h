#ifndef __REQUEST_H__

struct stats
{
    struct timeval arrival_time;
    struct timeval dispatch_interval;
    int handler_thread_id;
    int handler_thread_req_count;
    int handler_thread_static_req_count;
    int handler_thread_dynamic_req_count;

};
typedef struct stats *Stats;

void requestHandle(int fd,Stats stats);

#endif
