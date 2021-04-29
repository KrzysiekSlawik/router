#ifndef TIMEHELPERS
#define TIMEHELPERS
#include <sys/time.h>
typedef struct timeval time;
extern void decrement_time(time *start, time *to_decrement);
extern int time_in_ms(time *time);
extern int select_stopwatch(int sockfd, fd_set *desriptors, time *timeout);

#endif