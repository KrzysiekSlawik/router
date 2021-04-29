#ifndef UDPSERVERCOMPONENT
#define UDPSERVERCOMPONENT

#define UDP_IN_PORT 54321
#include <stdint.h>
#include <arpa/inet.h>
typedef void (*on_recv_func)(long, const uint8_t *, struct sockaddr *);
extern void init_udp_server();
extern void listen_udp(struct timeval *listen_for, on_recv_func on_recv_func);

#endif 