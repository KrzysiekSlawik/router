#include "udpServerComponent.h"
#include "udpClientComponent.h"
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
    printf("Im talking with me\n");
    if(len == 3 && strcmp((char*)buff, "ok") == 0)
    {
        recv_response_handler(sender);
    }
    else if(len == 9)
    {
        recv_route_handler(buff, sender);
    }
}

int main()
{
    call_inits();
    int n;
    if(scanf("%d", &n)<1)
    {
        fprintf(stderr, "invalid format error\n");
        exit(EXIT_FAILURE); 
    }
    
    for(int i = 0; i < n; i++)
    {
        address a = scan_cidr();
        
        route r; r.state = REACHABLE; r.destination = a; r.age = 0;
        if(scanf(" distance %d", &r.distance)<1)
        {
            fprintf(stderr, "invalid format error\n");
            exit(EXIT_FAILURE);
        }
        add_route(r);
    }
    for(;;)
    {
        call_updates();
    }
    return 0;
}
void call_inits()
{
    init_udp_server();
    init_udp_client();
    init_route_table();
}
void call_updates()
{
    struct timeval timeout;
    timeout.tv_sec=15; timeout.tv_usec=0;
    listen_udp(&timeout,recv_handler);
    update_route_table();
    print_route_table();
    update_udp_client();
}
