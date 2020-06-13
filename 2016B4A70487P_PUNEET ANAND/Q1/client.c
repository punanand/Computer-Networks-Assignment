/************************************************
Author:
Name	Puneet Anand
ID 		2016B4A70487P
************************************************/

#include "packet.h"

/**
 * @param offset	the offset of the data to be read from the file
 * @param channel	the channel through which the packet will be sent
 * @param fd 		the file descriptor of the file that is being uploaded
 * makes and returns a new packet with the following fields. 
 */
Packet makePacket(int * offset, int channel, int fd) {

	Packet new;
	new.seq_no = *offset;
	new.channel = channel;
	new.tag = 0;
	new.is_last = 0;

	char buff[PACKET_SIZE + 1];

	int n = read(fd, buff, PACKET_SIZE);
	if(n == 0)
		new.is_last = 1;

	new.size = n;
	strcpy(new.payload, buff);
	new.payload[n] = '\0';

	*offset = *offset + n;
	return new;
}

/** 
 * @param channel 	the channel through which the created packet will be sent
 * created an end marker for the server to close the client socket corresponding to 'channel'
 */
Packet makeEndPacket(int channel) {

	Packet new;
	new.is_last = 1;
	new.channel = channel;
	strcpy(new.payload, "");
	new.size = 0;
	return new;
}

/**
 * @param sec 	Number of seconds before timeout
 * @param usec  Number of micro-seconds before timeout
 * returns a timeval structure containing of 
 */
struct timeval makeTimeVal(int tot_usec) {

	/* initializing the timeval structure */
	struct timeval timeout;
	timeout.tv_sec = tot_usec / 1000000;
	timeout.tv_usec = tot_usec % 1000000;
	return timeout;
}

/* Reporting error and exiting */
void die(char * s) {
	perror(s);
	exit(1);
}

int main() {

	/* declarationd */
	int sock1, sock2, conn1, conn2, receiveto1, receiveto2;

	/* declarations for file handling */
	int fd, offset = 0, readfd;
	struct timeval timeout, sendtime1, sendtime2;
	int bytes_sent1, bytes_sent2, bytes_recieved1, bytes_recieved2;
	int done1 = 0, done2 = 0;
	int timeout_channel = 1;
	int sent_last = 0;
	
	/* Opening the two sockets and connecting to the server to the server */
	if((sock1 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		die("Socket1 not opened\n");
	}
	if((sock2 = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		die("Socket2 not opened\n");
	}

	/* defining sockaddr_in structure */
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	/* establishing connections */
	conn1 = connect(sock1, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	if(conn1 < 0) {
		die("Connection 1 not established\n");
	}
	conn2 = connect(sock2, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
	if(conn2 < 0) {
		die("Connection 2 not established\n");
	}

	fd = open("input.txt", O_RDWR, 0777);

	/* Declarations for the packets that will stoe the packet to be sent from respective channels */
	Packet p1, p2;

	p1 = makePacket(&offset, 1, fd);
	p2 = makePacket(&offset, 2, fd);

	if(p2.size == 0)
		sent_last = 1;

	/* initially sending the packets */
	bytes_sent1 = send(sock1, &p1, sizeof(p1), 0);
	printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
		p1.seq_no, p1.size, p1.channel);
	if(p1.is_last) {
		done2 = 1;
		sent_last = 1;
	}
	if(!done2) {
		bytes_sent2 = send(sock2, &p2, sizeof(p2), 0);
		printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
			p2.seq_no, p2.size, p2.channel);
		if(p2.is_last) {
			sent_last = 1;
		}
	}

	gettimeofday(&sendtime1, NULL);
	gettimeofday(&sendtime2, NULL);
	timeout = makeTimeVal(RETRANS_TIME * 1000000);

	while(1) {

		if(done1 && done2) {
			break;
		}

		/*initializing an fd_set */
		fd_set readfds;
		FD_ZERO(&readfds);

		/* insertinng the 2 socket descriptors to the fd_set */
		FD_SET(sock1, &readfds);
		FD_SET(sock2, &readfds);

		/* Calling select and noting the time taken by the call */
		struct timeval start, end;
		gettimeofday(&start, NULL);
		int ret = select(sock2 + 1, &readfds, NULL, NULL, &timeout);
		gettimeofday(&end, NULL);

		if(ret == -1) {
			/* error occured in the select call */
			die("Error in select\n");
		}
		else if(ret) {
			/* Niether error nor timeout occured */
			long long int seconds = (end.tv_sec - start.tv_sec);
			long long int micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
			Packet recieved_packet;
			if(FD_ISSET(sock1, &readfds)) {
				bytes_recieved1 = recv(sock1, &recieved_packet, sizeof(Packet), 0);
				printf("RCVD ACK: for PKT with Seq. No. %d from channel %d\n", 
					recieved_packet.seq_no, recieved_packet.channel);
				Packet new = makePacket(&offset, 1, fd);
				if(new.size == 0 && sent_last) {
					done1 = 1;
					continue;
				}
				else if(new.size == 0) {
					sent_last = 1;
				}
				p1 = new;
				bytes_sent1 = send(sock1, &p1, sizeof(p1), 0);
				printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
					p1.seq_no, p1.size, p1.channel);
				gettimeofday(&sendtime1, NULL);
				
				long long int tmp = (end.tv_sec - sendtime2.tv_sec);
				tmp += (end.tv_usec) - (sendtime2.tv_usec);
				tmp = RETRANS_TIME * 1000000 - tmp;
				timeout = makeTimeVal(tmp);
				timeout_channel = 2;
			}

			else if(FD_ISSET(sock2, &readfds)) {
				bytes_recieved2 = recv(sock2, &recieved_packet, sizeof(Packet), 0);
				printf("RCVD ACK: for PKT with Seq. No. %d from channel %d\n", 
					recieved_packet.seq_no, recieved_packet.channel);
				Packet new = makePacket(&offset, 2, fd);
				if(new.size == 0 && sent_last) {
					done2 = 1;
					continue;
				}
				else if(new.size == 0) {
 					sent_last = 1;
				}
				p2 = new;
				bytes_sent2 = send(sock2, &p2, sizeof(p2), 0);
				printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
					p2.seq_no, p2.size, p2.channel);
				gettimeofday(&sendtime2, NULL);
				
				long long int tmp = (end.tv_sec - sendtime1.tv_sec);
				tmp += (end.tv_usec) - (sendtime1.tv_usec);
				tmp = RETRANS_TIME * 1000000 - tmp;
				timeout = makeTimeVal(tmp);
				timeout_channel = 1;
			}
		}
		else {
			/* timeout occurred for timeout_channel */
			
			if(timeout_channel == 1) {
				if(!done1) {
					// printf("Timeout on channel %d\n", timeout_channel);
					bytes_sent1 = send(sock1, &p1, sizeof(p1), 0);
					printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
						p1.seq_no, p1.size, p1.channel);
				}
				gettimeofday(&sendtime1, NULL);
				long long int tmp = (end.tv_sec - sendtime2.tv_sec);
				tmp += (end.tv_usec) - (sendtime2.tv_usec);
				tmp = RETRANS_TIME * 1000000 - tmp;
				timeout = makeTimeVal(tmp);
				timeout_channel = 2;
			}
			else {
				if(!done2) {
					// printf("Timeout on channel %d\n", timeout_channel);
					bytes_sent2 = send(sock2, &p2, sizeof(p2), 0);
					printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
						p2.seq_no, p2.size, p2.channel);
				}
				gettimeofday(&sendtime2, NULL);
				long long int tmp = (end.tv_sec - sendtime1.tv_sec);
				tmp += (end.tv_usec) - (sendtime1.tv_usec);
				tmp = RETRANS_TIME * 1000000 - tmp;
				timeout = makeTimeVal(tmp);
				timeout_channel = 1;
			}
		}
	}
	close(sock1);
	close(sock2);
}