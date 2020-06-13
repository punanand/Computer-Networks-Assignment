/************************************************
Author:
Name	Puneet Anand
ID 		2016B4A70487P
************************************************/

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

/* defining the port number for the application */
#define PORT 12345

/* defines the size of the buffer in the multiples of packet size */
#define BUFFER_SIZE 5

#define MAXPENDING 5

typedef struct Packet {
	/* number of bytes of payload */
	int size;
	/* (bytes) offset of the first byte of the packet wrt input file */
	int seq_no;
	/* indicator set to 1 for the last packet */
	int is_last;
	/* identifier for DATA or ACK packet 0: DATA, 1:ACK */
	int tag;
	/* channel number, may be 0 or 1 */
	int channel;

	/* payload in the packet */
	char payload[PACKET_SIZE + 1];
} Packet;