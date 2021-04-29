#ifndef ROUTETABLECOMPONENT
#define ROUTETABLECOMPONENT

#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdint.h>

typedef enum
{
    NULLROUTE    = 0,
    UNREACHABLE  = 1,
    REACHABLE    = 2,
    REACHABLEVIA = 3
}routeState;

typedef struct
{
    uint32_t ip;
    uint8_t mask;
}address;

typedef struct
{
    routeState state;
    address destination;
    uint32_t distance;
    address via;
    uint8_t age;
}route;

extern void init_route_table();
extern void add_route(route);
extern void invalidate_route(address a);
extern void print_route_table();
extern void update_route_table();

#endif