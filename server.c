#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sysexits.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "rpc.pb-c.h"

#define MAXLOAD 20
struct server_arguments{
	int ports;
}

struct client_arguments{
	uint8_t *msg;
	size_t msg_len;
	uint32_t sum;
	int status; /* 0/1 disconnected / connected */
}

error_t server_parser(int key, char *arg, struct argp_state *state) {
	struct server_arguments *args = state->input;
	error_t ret = 0;
	switch (key) {
	case 'p':
		args->port = atoi(arg);
		if (args->port == 0) { 
			argp_error(state, "Invalid option for a port, must be a number");
		}
		break;
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}

void *server_parseopt(struct server_arguments *args, int argc, char *argv[]) {
	memset(args, 0, sizeof(*args));

	struct argp_option options[] = {
		{ "port", 'p', "port", 0, "The port to be used for the server" , 0 },
		{0}
	};
	
	struct argp argp_settings = { options, server_parser, 0, 0, 0, 0, 0 };
	if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
		fputs("Got an error condition when parsing\n", stderr);
		exit(EX_USAGE);
	}
	if (!args->port) {
		fputs("A port number must be specified\n", stderr);
		exit(EX_USAGE);
	}

	return args;
}

uint32_t addFunction(uint32_t a, uint32_t b){
	return a + b;
}

int addWrapper(uint8_t **valueSerial, size_t *valueSerialLen, const uint8_t *argsSerial, size_t argsSerialLne){
		int error = 0;
		//Deserialize/Unpack the arguments
		AddArguments *args = add_arguments__unpack(NULL, argsSerialLen, argsSerial);
		if(args == NULL) {
			return 1;
		}
		
		//Call the underlying function
		uint32_t sum = addFunction(args->a, args->b);
		*total += sum;
		
		//Serialize/Pack the return value
		AddReturnValue value = ADD_RETURN_VALUE__INIT;
		value.sum = sum;
		
		*valueSerialLen = add_return_value__get_packed_size(&value);
		*valueSerial = (uint8_t *)malloc(*verialSerialLen);
		if(*valueSerial == NULL){
			return 1;
		}
		add_return_value__pack(value, *valueSerial);
		
		//Cleanup
		add_arguments__free_unpacked(args, NULL);
		
		return error;
}

int gettotalWrapper(uint8_t **valueSerial, size_t *valueSerial_len, uint32_t *gettotal){
	int error = 0;
	//Serialize/Pack 
	AddReturnValue value = ADD_RETURN_VALUE__INIT;
	value.sum = *gettotal;
	
	*valueSerialLen = add_return_value__get_packed_size(&gettotal);
	*valueSerial = (uint8_t *)malloc(*valueSerialLen);
	if(*valueSerial == NULL){
		return 1;
	}
	add_return_value__pack(value, *valueSerial);
	
	return error;
}
	
}

int handleCall(uint8_t **retSerial, const uint8_t *callSerial, struct client_frame *current){
		int error = 0;
		
		//Deserializing/Unpacking the call
		size_t callSerialLen = ntohl(*(uint32_t *)callSerial);
		Call *call = call_unpack(NULL, callSerialLen, callSerial + 4);
		if(call == NULL){
			return 1;
		}
		
		// Calling the appropriate wrapper function based on the 'name' field
		uint8_t *valueSerial;
		size_t valueSerialLen;
		bool success;
		
		if (strcmp(call->name, "add") == 0) {
			success = !addWrapper(&valueSerial, &valueSerialLen, call->args.data, call->args.len);
		} else if(strcmp(call->name, "gettotal") == 0){
			success = !gettotalWrapper(&valueSerial, &valueSerialLen, call->args.data, call->args.len);
		} else {
			error = 1;
			goto errInvalidName;
		}
		
		//Serializing/Packing the return, using the return value from the invoked function
		Return ret = RETURN__INIT;
		ret.success = success;
		if (success) {
			ret.value.data = valueSerial;
			ret.value.len = valueSerialLen;
		}

		size_t retSerialLen = return__get_packed_size(&ret);
		*retSerial = (uint8_t *)malloc(4 + retSerialLen);
		if (*retSerial == NULL) {
			error = 1;
			goto errRetMalloc;
		}
		
		*(uint32_t *)*retSerial = htonl(retSerialLen);
		
		return_pack(&ret, *retSerial + 4);
		
		//Cleanup
		errRetMalloc:
					if (success) {
						free(valueSerial);
					}
		errInvalidName:
					call__free_unpacked(call, NULL);

		return error;
	
}


int handleConnection(int servSock, struct client_arguments *current){
	struct sockaddr_in clntAddr;
	socklen_t clntAddrLen = sizeof(clntAddr);
	
	int clntSock;
	if((clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen)) < 0 ){
		perror("accept() failed");
		exit(1);
	}
	fcntl(clntSock, F_SETFL, O_NONBLOCK);
	
	memset(current, 0, sizeof(*current));
	current->msg = malloc(MAXLOAD);
	
	return clntSock;
}


int main(int argc, char *argv[]){
	
	struct server_arguments args;
	server_parseopt(&args, argc, argv);
	
	struct pollfd *pollfds = malloc((MAXLOAD) * sizeof(struct pollfd));
	struct client_arguments **clients = malloc(MAXLOAD * sizeof(void *));
	
	int servSock;
	if((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("socket() failed\n");
		exit(1);
	}
	fcntl(servSock, F_SETFL, O_NONBLOCK);
	
	pollfds[0].fd = servSock;
	pollfds[0].events = POLLIN | POLLPRI;
	
	for(int i = 0; i < MAXLOAD; i++){
		pollfds[i+1].fd = -1;
	}
	
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(args.port);
	
	if(bind(servSock, (strcut sockaddr *)&servAddr, sizeof(servAddr)) < 0){
		perror("bind() failed");
		exit(1);
	}
	
	if(listen(servSock, SOMAXCONN) < 0){
		perror("listen() failed");
		exit(1);
	}
	
	int has_connection;
	ssize_t numOfBytes;
	while(1){
		switch(poll(pollfds, MAXLOAD, -1)){
			case -1:
					perror("poll() failed");
					exit(1);
			case 0:
					break;
			default:
					has_connection = pollfds[0].revents & POLLIN;
					for(int i = 0; i < MAXLOAD; i++){
						if(pollfds[i + 1].fd < 0){
							if(has_connection){
								clients[i] = malloc(sizeof(struct client_arguments));
								pollfds[i + 1].fd = handleConnection(servSock, clients[i]);
								pollfds[i + 1].events = POLLIN;
								has_connection = 0;
							} else {
								continue;
							}
						}
						
						if(pollfds[i + 1].revents & POLLIN){
							size_t bufLen = ntohl(*(uint32_t *)clients[i]->msg);
							size_t expBytes;
							if(clients[i]->msg_len < 4){
								expBytes = 4 - clients[i]->msg_len;
							} else {
								expBytes = 4 + bufLen - clients[i]->msg_len;
							}
							
							if(clients[i]->msg_len + expBytes > MAXLOAD){
								clients[i]->status = 1;
							} else if (expBytes){
								numOfBytes = recv(pollfds[i + 1].fd, clients[i]->msg + clients[i]->msg_len,expBytes, 0);
								if(numOfBytes <= 0) {
									if(numOfBytes){
										perror("recv() failed");
									} 
									clients[i]->status = 1;
								else {
									clients[i]->msg_len += numOfBytes;
									bufLen = ntohl(*(uint32_t *)clients[i]->msg);
								}
							}
							if(clients[i]->msg_len >= 4 && clients[i]->msg_len == bufLen + 4){
								uint8_t *retSerial;
								pollfds[i + 1].events &= ~POLLIN;
								if(handleCall(&retSerial, clients[i]->msg, clients[i])){
									clients[i]->status = 1;
								} else {
									clients[i]->msg_len = 4 + ntohl(*(uint32_t *)retSerial);
									memcpy(clients[i]->msg, retSerial, clients[i]->msg_len);
									free(retSerial);
									pollfds[i + 1].events |= POLLOUT;
								}
							}
						}
						
						if(pollfds[i+1].revents & POLLOUT) {
							numOfBytes = send(ufds[i + 1].fd, clients[i]->msg, clients[i]->msg_len, 0);
							if(numOfBytes <= 0){
								if(numOfBytes <= 0){
									perror("recv() failed");
								}
								clients[i]->status = 1;
							} else {
								clients[i]->msg_len -= numOfBytes;
								memmove(clients[i]->msg, clients[i]->msg + numOfBytes, clients[i]->msg_len);
							}
							if(!clients[i]->msg_len){
								pollfds[i + 1].events &= ~POLLOUT;
								pollfds[i + 1].events |= POLLIN;
							}
						}
						
						if(clients[i]->status){
							close(pollfds[i + 1].fd);
							pollfds[i + 1].fd = -1;
							free(clients[i]->msg);
							free(clients[i]);
						}
					}
					
					break;
				}
							
		}					
									
		
	}
}
	
	