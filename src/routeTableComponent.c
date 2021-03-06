#include "routeTableComponent.h"
#include "udpClientComponent.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TABLE_SEPARATOR "===========================================================\n"
#define RIP_AGE_DIRECT 4 /* RIP for requiescat in pace */
#define RIP_AGE_INDIRECT 2 /* not to be confused with routing information protocol */
#define MAX_DISTANCE 20
/*
 *  This component could use some serious performance improvements
 *  tho it works fine for small network sizes (O(n) time complexity)
 */
static route *routes;
static int table_size = 8;
void init_route_table()
{
    routes = calloc(table_size, sizeof(route));
    if(!routes)
    {
        fprintf(stderr, "route table allocation error\n");
        exit(EXIT_FAILURE);
    }
}

void add_route(route a)
{
    int nullroute_ind = -1;
    for(int i = 0; i < table_size; i++)
    {
        if(routes[i].destination.ip == a.destination.ip)
        {
            if(routes[i].distance >= a.distance || routes[i].state == UNREACHABLE)
            {
                routes[i] = a;
            }
            if(routes[i].via.ip == a.via.ip)
            {
                routes[i] = a;
            }
            return;
        }
        if(routes[i].state == NULLROUTE)
        {
            nullroute_ind = i;
        }
    }
    if(nullroute_ind != -1)
    {
        routes[nullroute_ind] = a;
    }
    table_size *= 2;
    route *new_table = realloc(routes, table_size*sizeof(route));
    if(new_table == NULL)
    {
        fprintf(stderr, "route table expanding error\n");
        free(routes);
        exit(EXIT_FAILURE);
    }
    routes = new_table;
    memset(routes+table_size/2, 0, table_size/2);
    routes[table_size/2] = a;
}


static void invalidate_routes_via(address via)
{
    for(int i = 0; i < table_size; i++)
    {
        if(routes[i].state == REACHABLEVIA && routes[i].via.ip == via.ip)
        {
            routes[i].state = NULLROUTE;
        }
    }
}

static void invalidate_route_on_index(int i)
{
    switch(routes[i].state)
    {
        case NULLROUTE:
            return;
        case UNREACHABLE:
            return;
        case REACHABLE:
            routes[i].distance = UINT32_MAX;
            routes[i].state = UNREACHABLE;
            invalidate_routes_via(routes[i].destination);
            return;
        case REACHABLEVIA:
            routes[i].state = NULLROUTE;
            return;
    }
}

void invalidate_route(address a)
{
    for(int i = 0; i < table_size; i++)
    {
        if(routes[i].destination.ip == a.ip && routes[i].destination.mask == a.mask)
        {
            invalidate_route_on_index(i);
        }
    }
    #ifdef DEBUG
    fprintf(stderr, "invalidated nonexistent route!\n");
    exit(EXIT_FAILURE);
    #endif
}
static void print_address(address a)
{
    unsigned char bytes[4];
    bytes[0] = a.ip & 0xFF;
    bytes[1] = (a.ip >> 8) & 0xFF;
    bytes[2] = (a.ip >> 16) & 0xFF;
    bytes[3] = (a.ip >> 24) & 0xFF;   
    printf("%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);        
}
void print_route_table()
{
    printf(TABLE_SEPARATOR);
    for(int i = 0; i < table_size; i++)
    {
        switch(routes[i].state)
        {
            case NULLROUTE:
                continue;
            case REACHABLEVIA:
                print_address(routes[i].destination);
                printf("/%d distance %d via", routes[i].destination.mask, routes[i].distance);
                print_address(routes[i].via);
                printf("\n");
                break;
            case REACHABLE:
                print_address(routes[i].destination);
                printf("/%d distance %d connected directly\n", routes[i].destination.mask, routes[i].distance);
                break;
            case UNREACHABLE:
                print_address(routes[i].destination);
                printf("/%d unreachable connected directly\n", routes[i].destination.mask);
                break;
        }
    }
}

static int is_too_old(route a)
{
    if(a.state == REACHABLE)
    {
        return a.age > RIP_AGE_DIRECT;
    }
    if(a.state == REACHABLEVIA)
    {
        return a.age > RIP_AGE_INDIRECT;
    }
    return 0;
}

void update_route_table()
{
    for(int i = 0; i < table_size; i++)
    {
        routes[i].age++;
        if(is_too_old(routes[i]))
        {
            invalidate_route_on_index(i);
        }
    }
}

static int match_masked_addr(address masked, address whole)
{
    int sufix_len = 32 - masked.mask;
    uint32_t mask = UINT32_MAX << sufix_len;
    masked.ip &= mask;
    whole.ip &= mask;
    return masked.ip == whole.ip;
}
void recv_route_handler(const uint8_t *buff, struct sockaddr *sender)
{
    struct sockaddr_in *s = (struct sockaddr_in*)sender;
    address via_addr; via_addr.ip = s->sin_addr.s_addr;
    address *dest_addr = (address*)buff;
    uint32_t distance = *((uint32_t*)(buff+5));
    for(int i = 0; i < table_size; i++)
    {
        if(match_masked_addr(routes[i].destination, via_addr))
        {
            distance += routes[i].distance;
            break;
        }
    }
    route r; r.destination = *dest_addr; r.distance = distance; r.age = 0; r.via = via_addr; r.state = REACHABLEVIA; 
    add_route(r);
    sendto_udp("ok", *sender);
}
void recv_response_handler(struct sockaddr *sender)
{
    struct sockaddr_in *s = (struct sockaddr_in*)sender;
    address addr; addr.ip = s->sin_addr.s_addr;
    for(int i = 0; i < table_size; i++)
    {
        if((routes[i].state == UNREACHABLE || routes[i].state == REACHABLE) 
            && match_masked_addr(routes[i].destination, addr))
        {
            routes[i].state = REACHABLE;
            routes[i].age = 0;
        }
    }
}
/* based on https://stackoverflow.com/questions/28532688/c-cidr-to-address-list */
address scan_cidr()
{
    address addr;
    uint8_t a, b, c, d, bits;
    if (scanf("%hhu.%hhu.%hhu.%hhu/%hhu", &a, &b, &c, &d, &bits) < 5)
    {
        fprintf(stderr, "invalid CIDR error\n");
        exit(EXIT_FAILURE);
    }
    if (bits > 32) 
    {
        fprintf(stderr, "invalid CIDR mask error\n");
        exit(EXIT_FAILURE);
    }
    addr.ip =
        (a << 24UL) |
        (b << 16UL) |
        (c << 8UL) |
        (d);
    uint32_t mask = (0xFFFFFFFFUL << (32 - bits)) & 0xFFFFFFFFUL;
    addr.ip &= mask;
    addr.mask = bits;

    return addr;
}