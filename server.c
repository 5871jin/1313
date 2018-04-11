#include <argp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>
#include "uthash.h"

#define ntohll htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))

static uint8_t msg[19]; /* time request  Ver:1 Seq:2 ClientSeconds:8  Client Nanoseconds: 8*/

struct server_arguments  {
	int port;
	double dropRate; /* Percentage chance value in [0,100]*/
};

struct client_arguments {
	struct sockaddr_in sockAddr;
	char *addr; /* the client address */
	int maxSeq; 
	uint8_t *buffer; /* outgoing info ?*/
	size_t buflen; /* outgoing length */
	time_t ttl;
	UT_hash_handle hh;
};


error_t server_parser(int key, char *arg, struct argp_state *state){
	struct server_arguments *args = state->input;
	error_t ret = 0;
	int prob;
	switch(key){
	case 'p':
		args->port = atoi(arg);
		if(args->port == 0){
			argp_error(state, "Port must be a number > 0");
		}else if (args->port <= 1024){
			argp_error(state, "Port must be greater than 1024");
		}
		break;
	case 'd':
		prob = atoi(arg);
		if(prob < 0 || prob > 100){
			argp_error(state, "Probability must be between 0~100");
		}
		args->dropRate = (double)prob / 100.0;
		break;
	default:	
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

void *server_parseopt(struct server_arguments *args, int argc, char *argv[]){
	memset(args, 0 , sizeof(*args));

	struct argp_option options[] = {
		{ "port", 'p', "port", 0, "The port bind between server and client", 0},
		{ "drop", 'd', "drop", 0, "The probability of dropping package", 0},
		{0}
	};

	struct argp argp_settings = {options, server_parser, 0 ,0 ,0 ,0 ,0 };

	if(argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0){
     		fputs("Got an error condition when parsing\n", stderr);
		exit(EX_USAGE);
	}

	if(!args->port){
		fputs("A port must be specified\n",stderr);
		exit(EX_USAGE);
	}
	return args;
}

void handleConnection(struct sockaddr_in *srcAddr, struct client_arguments *current){
	memset(current, 0, sizeof(*current));/* Zero out structure */

	memcpy(&current->sockAddr, srcAddr, sizeof(current->sockAddr)); /* store client address */

	current->addr = malloc(INET_ADDRSTRLEN + 6); /* store client address */
	inet_ntop(AF_INET, &current->sockAddr.sin_addr.s_addr, current->addr, INET_ADDRSTRLEN);
	sprintf(current->addr + strlen(current->addr), ":%d", ntohs(srcAddr->sin_port));
	current->addr = realloc(current->addr, strlen(current->addr) + 1);
	current->buffer = malloc(35);
}

int handleUDPClient(int sock, struct client_arguments **clients){

	struct client_arguments *current;
	struct sockaddr_in srcAddr;
	socklen_t srclen = sizeof(srcAddr);
	memset(&srcAddr, 0 , srclen);

	struct timespec t1;
	clock_gettime(CLOCK_REALTIME, &t1);

	if(recvfrom(sock, msg, 19, 0, (struct sockaddr *)&srcAddr, &srclen) < 0){
		perror("recvfrom() failed");
		exit(1);
	}

	HASH_FIND(hh, *clients, &srcAddr, srclen, current);

	if(current == NULL){
		current = malloc(sizeof(struct client_arguments));
		handleConnection(&srcAddr, current);
		HASH_ADD(hh, *clients, sockAddr, srclen, current);
		puts("handling incoming client\n");
	}

	uint8_t *buffer = current->buffer;
	memcpy(buffer, msg, 19);
	int seq = ntohl(*(uint16_t *)&buffer[1]); /* after version */

	if(current->maxSeq < seq){
		printf("%s:%u %d %d \n", current->addr,current->sockAddr.sin_port, seq, current->maxSeq);
		current->maxSeq = seq;
		current->ttl = 2;
	}

	*(uint64_t *)&buffer[19] = htonll((uint64_t)t1.tv_sec);
	*(uint64_t *)&buffer[27] = htonll((uint64_t)t1.tv_nsec);


	return 0;
}

void handleSending(int sock, struct client_arguments **clients){
	struct client_arguments *current, *temp;
	HASH_ITER(hh, *clients, current, temp){
		struct sockaddr_in *sockAddr = &current->sockAddr;
		uint8_t *buffers = current->buffer;
		if(sendto(sock, buffers, 35, 0, (struct sockaddr *)sockAddr, sizeof(*sockAddr)) < 0){
			perror("send() failed");
			exit(1);
		}
		puts("sended response");
	}
}

int main(int argc, char *argv[]){
	
	struct client_arguments *clients = NULL;
	struct server_arguments args;
	server_parseopt(&args, argc, argv);

	srand(time(NULL));
	
	struct pollfd pollfds;
	
	int sockfd;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		perror("socket() failed");
		exit(1);
	}
	pollfds.fd = sockfd;
	
	pollfds.events = POLLIN | POLLPRI;
	fcntl(pollfds.fd, F_SETFL, O_NONBLOCK);

	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(args.port);

	if(bind(pollfds.fd,(struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
		perror("bind() failed");
		exit(1);
	}

	time_t timeout = -1;
	while(1){
	switch(poll(&pollfds, 1, timeout)){
	case -1:
		perror("poll() failed");
		exit(1);
	case 0:
		break;
	default:
		if(pollfds.revents & POLLIN) {
			if(rand() >= args.dropRate * ((double)RAND_MAX + 1.0)){
				if(handleUDPClient(pollfds.fd, &clients) <1){
					pollfds.events |= POLLOUT & ~POLLIN;
				}

			}else{
				puts("dropping packet");
				recvfrom(pollfds.fd, msg, 19, 0, NULL, 0);

			}
		}

		if(pollfds.revents & POLLOUT){
			handleSending(pollfds.fd, &clients);
			pollfds.events |= POLLIN &~POLLOUT;

		}

		break;
	}
   }
}
