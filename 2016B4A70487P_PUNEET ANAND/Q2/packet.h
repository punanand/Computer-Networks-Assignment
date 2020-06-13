/************************************************
Author:
Name	Puneet Anand
ID 		2016B4A70487P
************************************************/

#ifndef _PACKET
#define _PACKET

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

/* the size of the payload ina given packet structure */
#define PACKET_SIZE 100

/* packet drop rate, to be taken care of at the server */
#define PDR 10

/* the time aftr which the packet will be retransmitted */
#define RETRANS_TIME 2

/* defining the port number for R1 */
#define PORTR1 8001

/* defining the port number for R2 */
#define PORTR2 8002

/* defining the port number for the server */
#define PORTSERVER1 8003

/* defining the port number 2 for the server */
#define PORTSERVER2 8004

/* window size for the selective repeat procedure */
#define WINDOW_SIZE 4

/* format specifier for printing the event logs */
#define FORMAT_SPEC "%-10s\t%-10s\t%-15s\t\t%-10s\t\t%-10d\t%-10s\t%-10s\n"

/* format specifier for printing headings of the event logs */
#define FORMAT_SPEC_HEAD "%-10s\t%-10s\t%-15s\t\t%-10s\t\t%-10s\t%-10s\t%-10s\n"

/* format specifier for printing the temporary event logs */
#define FORMAT_SPEC_TEMP "%s %s %lld %s %d %s %s %s\n"

typedef struct Packet {
	/* number of bytes of payload */
	int size;
	/* (bytes) offset of the first byte of the packet wrt input file */
	int seq_no;
	/* indicator set to 1 for the last packet */
	int is_last;
	/* identifier for DATA or ACK packet 0: DATA, 1:ACK */
	int tag;
	/* identifier for odd or even packet 0: even, 1: odd */
	int is_odd;

	/* payload in the packet */
	char payload[PACKET_SIZE + 1];
} Packet;

/* declaring a node structure which would help us maintain a linked list og logs to be printed */
typedef struct Node {

	long long int key;
	char me[20];
	char event_type[5];
	char packet_type[20];
	int seq;
	char src[10];
	char dest[10];
	char currtime[20];
	struct Node * next;
} Node;

Node * head;

#endif