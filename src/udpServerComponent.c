#include "udpServerComponent.h"
#include "timeHelpers.h"

#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

struct udp_server_state{
    int sockfd;
    struct sockaddr_in server_address;
    fd_set descriptors;
    struct sockaddr last_recv_from;
    ssize_t last_msg_len;
    uint8_t buffer[IP_MAXPACKET];
}state;

void init_udp_server()
{
    state.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (state.sockfd < 0) {
		fprintf(stderr, "socket error: %s\n", strerror(errno)); 
		exit(EXIT_FAILURE);
	}

	bzero (&state.server_address, sizeof(state.server_address));
	state.server_address.sin_family      = AF_INET;
	state.server_address.sin_port        = htons(UDP_IN_PORT);
	state.server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (state.sockfd, (struct sockaddr*)&state.server_address, sizeof(state.server_address)) < 0) {
		fprintf(stderr, "bind error: %s\n", strerror(errno)); 
		exit(EXIT_FAILURE);
	}
}

void listen_udp(struct timeval *listen_for, on_recv_func on_recv)
{
    int ready;
    socklen_t sender_len = sizeof(state.last_recv_from);
    FD_ZERO(&state.descriptors);
    FD_SET(state.sockfd, &state.descriptors);
    while((ready = select_stopwatch(state.sockfd+1, &state.descriptors, listen_for)) > 0)
    {
        if((state.last_msg_len = recvfrom(state.sockfd, &state.buffer, IP_MAXPACKET, 0, (struct sockaddr*)&state.last_recv_from, &sender_len))>=0)
        {
            on_recv(state.last_msg_len, state.buffer, &state.last_recv_from);
        }
        else
        {
            fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    if (ready < 0)
    {
        fprintf(stderr, "select error: %s\n", strerror(errno)); 
		exit(EXIT_FAILURE);
    }
}
ssize_t icmp_recv(int soc, uint8_t* buff, struct sockaddr_in* sender, struct timeval* wait)
{
    socklen_t sender_len = sizeof(*sender);
    int status = recvfrom(soc, buff, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr*)sender, &sender_len);
    if (status < 0)
    {
        if (errno != EWOULDBLOCK)
        {
            fprintf(stderr, "recvfrom error: %s\n", strerror(errno)); 
			exit(EXIT_FAILURE);
        }
        fd_set descriptors;
        FD_ZERO(&descriptors);
        FD_SET(soc, &descriptors);
        struct timeval time_before_wait;
        gettimeofday(&time_before_wait, NULL);
        int ready = select(soc+1, &descriptors, NULL, NULL, wait);
        if (!ready)
        {
            return 0;
        }
        if (ready < 0)
        {
            fprintf(stderr, "select error: %s\n", strerror(errno)); 
			exit(EXIT_FAILURE);
        }
        struct timeval time_after_wait;
        gettimeofday(&time_after_wait, NULL);
        struct timeval elpased_time;
        timersub(&time_after_wait, &time_before_wait, &elpased_time);
        timersub(wait, &elpased_time, wait);
        return icmp_recv(soc, buff, sender, wait);
    }
    return status;
}

int get_udp_socket()
{
    return state.sockfd;
}