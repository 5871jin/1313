#include <argp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>
#define ntohll htonll
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))

static uint8_t buffers[2048];

struct client_arguments {
	struct sockaddr_in servAddr; /* server address */
	int numOfReq; /* number of TimeRequests */
	time_t timeout; /* Timeout */
};

struct timereq{
	int ttl; /* time-to-live */
	double theta;
	double delta;
	int status; /* 0 for sending TimeRequest
		       1 for request timeout
		       2 for TimeRequest received */
};


error_t client_parser(int key, char *arg, struct argp_state *state){
	struct client_arguments *args = state->input;
	error_t ret = 0;
	int temp;
	switch(key) {
	case 'a':
		memset(&args->servAddr, 0, sizeof(args->servAddr));/* Zero out structure */
		args->servAddr.sin_family = AF_INET; /* Internet addr family */
		if(!inet_pton(AF_INET, arg, &args->servAddr.sin_addr.s_addr)){
			argp_error(state, "Invalid address");
		}
		break;
	case 'p':
		temp = atoi(arg);
		if(temp <= 0) {
			argp_error(state, "Invalid option for a port, must be a number");
		}
		args->servAddr.sin_port = htons(temp); /* Use given port */
		break;
	case 'n':
		args->numOfReq = atoi(arg);
		if(args->numOfReq < 0) {
			argp_error(state, "Number of TimeRequests must be >= 0");
		}
		break;
	case 't':
		args->timeout = 1000 * atoi(arg);
		if(args->timeout < 0){
			argp_error(state, "Timeout must be a number >= 0");
   		}
		if(!args->timeout){
			args->timeout = -1;
		}
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

void client_parseopt(struct client_arguments *args, int argc, char *argv[]) {
	struct argp_option options[] = {
		{ "addr", 'a', "addr", 0, "The IP address the server is listening at", 0},
		{ "port", 'p', "port", 0, "The port that is being used at the server", 0},
		{ "timereq", 'n', "timereq", 0, "The number of time requests to send to the server", 0},
		{ "timeout", 't', "timeout", 0, "The time in seconds to wait after sending its lats TimeRequest to receive a TimeResponse",0},	       
		{0}
	};

	struct argp argp_settings = { options, client_parser, 0, 0, 0, 0, 0 };

	memset(args, 0, sizeof(*args));
	args->numOfReq = -1;
	args->timeout = -1;

	if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
		printf("Got error in parse\n");
		exit(1);
	}

	if(!args->servAddr.sin_addr.s_addr){
		fputs("Server address must be specified \n", stderr);
		exit(EX_USAGE);
	}

	if(!args->servAddr.sin_port){
		fputs("Port must be specified\n", stderr);
		exit(EX_USAGE);
	}

	if(args->numOfReq < 0){
		fputs("Number of TimeRequests must be specified\n", stderr);
		exit(EX_USAGE);
	}

	if(args->timeout < 0){
		fputs("Timeout must be specified\n", stderr);
		exit(EX_USAGE);
	}
}

int main(int argc, char *argv[]){
	

	struct client_arguments args;
	client_parseopt(&args, argc, argv);
	srand(time(NULL));

	int sockfd;
	struct pollfd pollfds;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);/* Create a datagram socket using UDP */
	if(sockfd < 0){
		perror("Create Socket Failed: ");
		exit(1);
	}
	pollfds.fd = sockfd; /* poll recv and send to socket*/

	pollfds.events = POLLIN | POLLOUT; /* monitor I/O status*/
	fcntl(pollfds.fd, F_SETFL, O_NONBLOCK); /*set file status to non-block */

	struct timespec t0; /* need for calculating theta */
	struct timereq *requests = malloc(args.numOfReq * sizeof(struct timereq)); /* initialize N TimeRequest/TimeResponses */

	/* initialize parameters in collection of TimeRequests*/
	for(int i = 0; i < args.numOfReq; i++){
		requests[i].status = 0; /* Set Request[0], Request[1],... to Sending TimeRequest status*/
		requests[i].ttl = -1; /* Set default time-to-live paramter */
	}

	int version = 1;
	int seqNum = 1;
	int timeout = -1;
	ssize_t numOfBytes;
	int flag = 0;

	time_t prevReq = time(NULL);/* Initial time for processing */
	time_t tInterval = 0; /* Time difference container */



	*(uint8_t *)buffers = htonl(version);
	for(flag = 0;	!flag;	prevReq += tInterval){
		switch(poll(&pollfds,1,timeout)){
		case -1:
			perror("poll() failed");
			exit(1);
		case 0:
		/* An event on one of the fds has occurred */
		default:
			tInterval = time(NULL) - prevReq; /* Time different between last TimeRequest and current time */

			for(int i = 0 ; i < args.numOfReq; i++){
				if(requests[i].ttl < 0){
					 continue; /* if time-to-live = -1, current TimeRequest is complete or not sent yet*/
				}

				if(requests[i].ttl - tInterval <= 0){ /* if time passed exceed the time-to-live period */
					requests[i].ttl = -1; /* set time to live to default */
					requests[i].status = 1; /* set status to TimeResponse Completed */

				} else if (requests[i].ttl -= tInterval < timeout || timeout == -1){ /* poll() end if Timeout reached */
					timeout = requests[i].ttl;
				} 
			}

			if(pollfds.revents & POLLIN){ /* ready for receiving TimeResponse */
				numOfBytes = recvfrom(pollfds.fd, buffers, 2048, 0, NULL, 0); /* receive from server */
			
				if(numOfBytes < 0){
					perror("recvfrom() failed");
					exit(1);
				}

				int recvSeqNum = ntohl(*(uint16_t *)&buffers[1]); /* two-byte Seq# for TimeResponse*/

				struct timereq *temp = &requests[recvSeqNum - 1]; /* Update TimeRequest Collection */

				struct timespec t2; /*When receiving TimeResponse, takes current time, refer it to T2*/
				clock_gettime(CLOCK_REALTIME, &t2);

				if(temp->status == 0){ /* if the TimeRequest in waiting status */
				/* Format
				-----------------------------------------------------------------------------------------
                                | Ver | Seq# | ClientSeconds | CLient Nanoseconds | Server Seconds | server Nanoseconds |
                                -----------------------------------------------------------------------------------------
                                | 1   |  2   |      8        |          8         |         8      |          8         |
				-----------------------------------------------------------------------------------------
                                 */
					time_t cs = ntohll(*(uint64_t *)&buffers[3]); /*ClientSeconds*/
					time_t ss = ntohll(*(uint64_t *)&buffers[19]);/*ServerSeconds*/
					time_t t2s = t2.tv_sec;/* current time seconds */

					long cns = ntohll(*(uint64_t *)&buffers[11]);/*Client Nanoseconds */
					long sns = ntohll(*(uint64_t *)&buffers[27]);/*Server Nanoseconds */
					long t2ns = t2.tv_nsec;

					time_t second = ss - cs + ss - t2s;
					long nanosecond = sns - cns + sns - t2ns;

					if(nanosecond < 0){
						nanosecond += 1000000000;
						second --;

					}
                                        /* theta = ((T1 - T0) + (T1 - T2))/2
                                           delta = T2 - T0 */
					temp->theta = (double)second / 2.0 + (double)nanosecond / 2000000000.0;
					temp->delta = (double)(t2ns - cs) + (double)(t2ns - cns) / 1000000000.0;
					temp->ttl = -1;
					temp->status = 2;
				}

			}

			if(pollfds.revents & POLLOUT){/* Ready to Send TimeRequest */
			
				clock_gettime(CLOCK_REALTIME, &t0);/* client current timestamp*/


			      /*----------------------------------------------------
				| Ver | Seq# | Client Seconds | Client Nanoseconds |
                                ----------------------------------------------------
				|  1  |  2   |        8       |          8         |
                                ---------------------------------------------------- */

				*(uint32_t *)&buffers[1] = htonl(seqNum);
				*(uint64_t *)&buffers[3] = htonll((uint64_t)t0.tv_sec);
				*(uint64_t *)&buffers[11] = htonll((uint64_t)t0.tv_nsec);

				numOfBytes = sendto(pollfds.fd, buffers, 19, 0, (struct sockaddr *)&args.servAddr, sizeof(args.servAddr));
				puts("sended request");
				if(errno != EAGAIN){ /* Non-blocking I/O. There is data available right now, fail at sending */
				if(numOfBytes < 0){
						perror("sendto() failed");
						exit(1);
					}
					requests[seqNum - 1].ttl = args.timeout; /* sending TimeRequest and COUNT DOWN!*/

					if(seqNum++ == args.numOfReq){ /* Stop monitoring/poll() for outstreaming */
						pollfds.events &= ~POLLOUT;
					}
				}
			}

			flag = 1;
			for(int i = 0; i < args.numOfReq; i++){
				flag &= requests[i].ttl == -1 && requests[i].status != 0;
			}
			break;
		}
		}
		for(int i = 0; i < args.numOfReq; i++){
			if(requests[i].status == 1){
				printf("%d: Dropped\n", i);
			} else {
				printf("%d: %.4f %.4f\n", i ,requests[i].theta, requests[i].delta);
			}
		}

		close(pollfds.fd);
		free(requests);
		puts("Finished");
       
}
