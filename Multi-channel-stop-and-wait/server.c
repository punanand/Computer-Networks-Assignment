/************************************************
Author:
Name	Puneet Anand
ID 		2016B4A70487P
************************************************/

#include "packet.h"

/* Buffers and its offset for ordering the packets */
char recieved_buffer[BUFFER_SIZE * PACKET_SIZE + BUFFER_SIZE + 1];
int offset_recieved;

/**
 * @param offset 	the sequence number of the acknowledgement packet
 * @param channel 	channel nunber through which ack will be sent
 * returns a newly created Packet record with the specified fields
 */
Packet makePacket(int offset, int channel) {

	Packet new;
	new.seq_no = offset;
	new.channel = channel;
	new.tag = 1;
	new.size = 0;
	return new;
}

/**
 * @param st 	the starting index of the buffer string to be populated
 * @param tmp 	the payload recieved and to be copied into the buffer
 * copies the string tmp at the reqired location in the buffer string 
 */
void copytobuffer(int st, char * tmp) {
	int i;
	for(i = 0; i < strlen(tmp); i++) {
		recieved_buffer[st + i - offset_recieved] = tmp[i];
	}
}

/* Reporting error and exiting */
void die(char * s) {
	perror(s);
	exit(1);
}

int main() {

	srand(time(0));

	/* declarations */
	int serversock;
	int clientsock1, clientsock2;
	int fd;
	int bytes_recieved, bytes_sent;
	int done1 = 0, done2 = 0;
	int recieved_last = 0;
	offset_recieved = 0;
	memset(recieved_buffer, '\0', BUFFER_SIZE * PACKET_SIZE + BUFFER_SIZE + 1);

	/* opening a server socket */
	serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(serversock < 0) {
		die("Server socket not created");
	}
	
	/* initializing the server and client addresses */
	struct sockaddr_in serverAddress, clientAddress1, clientAddress2;
	
	/* assigning the server address */
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	/* binding the server address to the sockets */
	int temp = bind(serversock, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
	if(temp < 0) {
		die("Binding 1 failed\n");
	}

	int temp1 = listen(serversock, MAXPENDING);
	if(temp1 < 0) {
		die("Error in listen 1\n");
	}

	/* accepting the connection from the client */
	int clientLength1 = sizeof(clientAddress1);
	clientsock1 = accept(serversock, (struct sockaddr*) &clientAddress1, &clientLength1);
	if(clientLength1 < 0) {
		die("Error in client socket 1\n");
	}
	int clientLength2 = sizeof(clientAddress2);
	clientsock2 = accept(serversock, (struct sockaddr*) &clientAddress2, &clientLength2);
	if(clientLength2 < 0) {
		die("Error in client socket 1\n");
	}

	fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);

	while(1) {

		fd_set readfds;
		FD_ZERO(&readfds);

		FD_SET(clientsock1, &readfds);
		FD_SET(clientsock2, &readfds);

		int ret = select(clientsock2 + 1, &readfds, NULL, NULL, NULL);

		if(ret <= 0) {
			die("Error in server select\n");
		}

		Packet recieved_packet;

		int r = (rand() % 100);
		
		if(FD_ISSET(clientsock1, &readfds)) {
			bytes_recieved = recv(clientsock1, &recieved_packet, sizeof(Packet), 0);
		}
		else {
			bytes_recieved = recv(clientsock2, &recieved_packet, sizeof(Packet), 0);
		}
		if(bytes_recieved < 0) {
			die("Error in recieve\n");
		}

		printf("RCVD PKT: Seq. No %d of size %d Bytes from channel %d\n", 
			recieved_packet.seq_no, recieved_packet.size, recieved_packet.channel);

		if(r < PDR) {
			/* nothing to do */
			continue;
		}

		if(recieved_packet.seq_no - offset_recieved >= (BUFFER_SIZE - 1) * PACKET_SIZE) {
			continue;
		}
		if(recieved_packet.is_last) {
			recieved_last = 1;
		}
		
		copytobuffer(recieved_packet.seq_no, recieved_packet.payload);

		Packet ack = makePacket(recieved_packet.seq_no, recieved_packet.channel);
		if(FD_ISSET(clientsock1, &readfds)) { 
			bytes_sent = send(clientsock1, &ack, sizeof(Packet), 0);
		}
		else {
			bytes_sent = send(clientsock2, &ack, sizeof(Packet), 0);
		}
		if(bytes_sent < 0) {
			die("Error in send\n");
		}
		printf("SENT ACK: for PKT with Seq. No. %d from channel %d\n", 
			ack.seq_no, ack.channel);

		if(recieved_packet.seq_no == offset_recieved) {
			int tmp = write(fd, recieved_buffer, strlen(recieved_buffer));
			offset_recieved += strlen(recieved_buffer);
			memset(recieved_buffer, '\0', strlen(recieved_buffer));
			if(recieved_last) {
				break;
			}
		}
	}
	close(serversock);
	close(clientsock1);
	close(clientsock2);
}