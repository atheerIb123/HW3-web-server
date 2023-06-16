#ifndef __REQUEST_H__

struct Stats
{
    struct timeval arrivalTime;
    struct timeval dispachInterval;
    int workingThreadsStaticCount;
    int workingThreadsDynamicCount;
    int workingThreadId;
    int requestCount;
};
typedef struct Stats* Stats;

void requestHandle(int fd, Stats st);

#endif
