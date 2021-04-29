#include "udpServerComponent.h"
#include "routeTableComponent.h"
#include "router.h"
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

void recv_handler(long len, const uint8_t *buff, struct sockaddr *sender)
{
    if(sender != NULL)
    {
        printf("Im alive\n len = %ld, %s\n", len, buff);
    }
}

int main()//int argc, char const *argv[])
{ 
    call_inits();
    for(;;)
    {
        call_updates();
    }
    return 0;
}
void call_inits()
{
    init_udp_server();
    init_route_table();
}
void call_updates()
{
    struct timeval timeout;
    timeout.tv_sec=15; timeout.tv_usec=0;
    listen_udp(&timeout,recv_handler);
    update_route_table();
    print_route_table();
}
