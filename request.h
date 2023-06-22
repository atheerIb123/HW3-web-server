#ifndef __REQUEST_H__
#define __REQUEST_H__

struct stats
{
    struct timeval arrival;
    struct timeval dispatch;
    int handlerId;
    int handlerReqCount;
    int handlerStaticReqCount;
    int handlerDynamicReqCount;

};
typedef struct stats* Stats;

void requestHandle(int fd,Stats stats);

#endif
