#include "timeHelpers.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void decrement_time(time *start, time *to_decrement)
{
    time end;
    gettimeofday(&end, NULL);
    time elpased_time;
    timersub(&end, start, &elpased_time);
    timersub(to_decrement, &elpased_time, to_decrement);
}
int time_in_ms(time *time)
{
    fprintf(stderr, "time_in_ms not implemented!\n");
    return time->tv_usec;
}
int select_stopwatch(int sockfd, fd_set *desriptors, time *timeout)
{
    time start;
    gettimeofday(&start, NULL);
    time timecpy = *timeout;
    /* feeding select with copy of timeout to be sure timeout wont be changed by select (as its undefined) */
    int status = select(sockfd+1, desriptors, NULL, NULL, &timecpy);
    if (status < 0)
    {
        fprintf(stderr, "select error: %s\n", strerror(errno)); 
		exit(EXIT_FAILURE);
    }
    decrement_time(&start, timeout);
    return status;
}