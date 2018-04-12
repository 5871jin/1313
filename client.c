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

struct client_arguments {
	struct sockaddr_in servAddr;
	int next_RPC; /*The time in seconds between add RPCs*/
	int report_total; /*The number of add RPCs between gettotal RPCs*/
};

int sock;
uint8_t msg[32];

error_t client_parser(int key, char *arg, struct argp_state *state) {
	struct client_arguments *args = state->input;
	error_t ret = 0;
	int port;
	
	switch (key) {
	case 'a':
		memset(&args->servAddr, 0, sizeof(args->servAddr)); 
		args->servAddr.sin_family = AF_INET;
		if (!inet_pton(AF_INET, arg, &args->servAddr.sin_addr.s_addr)) {
			argp_error(state, "Invalid address");
		}
		break;
	case 'p':
		port = atoi(arg);
		if (port <= 0) {
			argp_error(state, "Invalid option for a port, must be a number greater than 0");
		}
		args->servAddr.sin_port = htons(port);
		break;
	case 't':
		args->next_RPC = atoi(arg);
		if (args->next_RPC < 0 || 30 < args->next_RPC) {
			argp_error(state, "This value must be in the range [0,30]");
		}
		break;
	case 'n':
		args->report_total = atoi(arg);
		if (args->report_total <= 0 || 10 < args->report_total) {
			argp_error(state, "This value must be a number in the range [1,10]");
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
		{ "addr", 'a', "addr", 0, "The IP address the server is listening at", 0 },
		{ "port", 'p', "port", 0, "The port that is being used at the server", 0 },
		{ "time", 't', "next_RPC", 0, "The time in seconds between add RPCs", 0 },
		{ "totaln", 'n', "report_total", 0, "The number of add RPCs between gettotal RPCs", 0 },
		{0}
	};

	struct argp argp_settings = { options, client_parser, 0, 0, 0, 0, 0 };

	memset(args, 0, sizeof(*args));
	args->next_RPC = -1;

	if (argp_parse(&argp_settings, argc, argv, 0, NULL, args) != 0) {
		printf("Got error in parse\n");
	}
	if (!args->servAddr.sin_addr.s_addr) {
		fputs("addr must be specified\n", stderr);
		exit(EX_USAGE);
	}
	if (!args->servAddr.sin_port) {
		fputs("port must be specified\n", stderr);
		exit(EX_USAGE);
	}
	if (args->next_RPC < 0) {
		fputs("the time between RPCs must be specified\n", stderr);
		exit(EX_USAGE);
	}
	if (!args->report_total) {
		fputs("number of add RPCs between gettotal RPCs must be specified\n", stderr);
		exit(EX_USAGE);
	}
}


/**
static uin32_t total = 0;
uint32_t add(uint32_t a, uint32_t b)
{
total += (a + b);
return (a + b);
}
uin32_t gettotal(void)
{
return (total);
}
**/
int calladd(uint32_t *sum, uint32_t a, uint32_t b){
	int error = 0;
	
	//Serializing/Packing the arugments
	AddArguments args = ADD_ARGUMENTS_INIT;
	args.a = a;
	args.b = b;
	
	size_t argsSerialLen = add_arguments_get_packed_size(&args);
	uint8_t *argsSerial = (uint8_t *)malloc(argsSerialLen);
	if(argsSerial == NULL){
		return 1;
	}
	
	add_arguments__pack(&args, argsSerial);
	
	// Serializing/Packing the call, which also placing the serialized/packed
	// arguments inside
	Call call = CALL__INIT;
	call.name = "add";
	call.args.len = argsSerialLen;
	call.args.data = argsSerial;
	
	size_t callSerialLen = call__get_packed_size(&call);
	uint8_t *callSerial = (uint8_t *)malloc(callSerialLen + 4); /*!!!!!CALL For Network Trans*/
	if (callSerial == NULL) {
		error = 1;
		goto errCallMalloc;
	}
	
	*(uint32_t *)callSerial = htonl(callSerialLen); /* !!!!!4 byte somthing */
	
	call__pack(&call, callSerial + 4);
	
	uint8_t *retSerial = msg; /* Retruan Serial across the network */
	if (handleCall(&retSerial, &retSerialLen, callSerial, callSerialLen)) {
		error = 1;
		goto errHandleCall;
	}
	
	// Deserialize/Unpack the return message
	Return *ret = return__unpack(NULL, ntohl(*(uint32_t *)retSerial), retSerial + 4);
		if (ret == NULL) {
		error = 1;
		goto errRetUnpack;
	}
	
	// Deserialize/Unpack the return value of the invert call
	AddReturnValue *value = add_return_value__unpack(NULL, ret->value.len, ret->value.data);
	if (value == NULL) {
		error = 1;
		goto errValueUnpack;
	}
	
	if (ret->success) {
		*sum = value->sum;
	} else {
		error = 1;
		printf("add(%d, %d) RPC failed!\n", a, b);
	}
	
	//Cleanup
	add_return_value__free_unpacked(value, NULL);
	errValueUnpack:
	return__free_unpacked(ret, NULL);
	errRetUnpack:
	free(callSerial);
	errCallMalloc:
	free(argsSerial);
	
	return error;
}

int callgettotal(uint32_t *sum){
	int error = 0;
	
	//Serializing
	Call call = CALL_INT;
	call.name = "getaddtotal";
	call.args.len = 0;
	call.args.data = NULL;
	
	size_t callSerialLen = call__get_packed_size(&call);
	uint8_t *callSerial = (uint8_t *)malloc(4 + callSerialLen);
	if(callSerial == NULL){
		error = 1;
		goto erCallMalloc;
	}
	
	*(uint32_t *)callSerial = htonl(callSerial_len);
	
	call__pack(&call, callSerial + 4);
	
	uint8_t *retSerial = msg;
	if(handleCall(retSerial, callSerial)){
		error = 1;
		goto errRetUnpack;
	}
	
	// Deserialize/Unpack the return value of the invert call
	AddReturnValue *value = add_return_value__unpack(NULL, ret->value.len, ret->value.data);
	if (value == NULL) {
		error = 1;
		goto errValueUnpack;
	}
	
	if (ret->success) {
		*sum = value->sum;
	} else {
		error = 1;
		printf("gettotal(%d) RPC failed!\n", v);
	}
	
	// Cleanup
	add_return_value__free_unpacked(value, NULL);
	errValueUnpack:
	return__free_unpacked(ret, NULL);
	errRetUnpack:
	free(callSerial);
	errCallMalloc:

	return error;
}
int handleCall(uint8_t *retSerial, const uint8_t *callSerial){
	int error = 0;
	ssize_t numOfBytes;
	
	send(sock, callSerial, 4 + ntohl(*(uin32_t *)callSerial), 0);
	numOfBytes = recv(sock, retSerial, 4, 0);
	if(numOfBytes <= 0){
		if(numOfBytes){
			perror("recv() failed\n");
			return 1;
		}
	}
	
	numOfBytes = recv(sock, retSerial + 4, ntohl(*(uint32_t *)retSerial, 0);
	
	if(numOfBytes <= 0){
		if(numOfBytes){
			perror("recv() failed\n");
			return 1;
		}
	}
	
	return error;
}
int main(int argc, char *argv[]){
	struct client_arguments args;
	
	client_parseopt(&args,argc,argv);
	srand(time(NULL)); /*Seed the random number generator*/
	
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		perror("socket() failed\n");
		exit(1);
	}
	
	if (connect(sock, (struct sockaddr *)&args.servAddr, sizeof(args.servAddr)) < 0) {
		perror("connect() failed");
		exit(1);
	}
	
	uin32_t a; /* first value for add function */
	uint32_t b; /* second value for add function */
	uint32_t sum; /* return value for add function */
	
	int flag = args.report_total; 
	
	for(;; sleep(args.next_RPC)){
		/*e two values to supply as arguments uniformly at random from [0,1000]*/
		a = rand() / ((double)RAND_MAX + 1.0) * 1001;
		b = rand() / ((double)RAND_MAX + 1.0) * 1001;
		calladd(&sum, a, b);
		printf("%d + %d = %d\n", a, b, sum); /*print a single line after the RPC completes*/
		
		if(!--flag) {
			callgettotal(&sum);
			printf("current total returned by server: %d\n", sum);
			flag = args.report_total;
		}
	}
}
	
	