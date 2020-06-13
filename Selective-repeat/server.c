/************************************************
Author:
Name	Puneet Anand
ID 		2016B4A70487P
************************************************/

#include "packet.h"

/* Buffers and its offset for ordering the packets */
char recieved_buffer[(WINDOW_SIZE + 1) * PACKET_SIZE];
int offset_recieved;

/* File pointer for storing the logs corresponding to server */
FILE * event;

/**
 * This function returns the current timestamp in string format
 */
char * get_local_time() {
	char * ret = (char *)malloc(20 * sizeof(char));
	time_t curr_time;
	struct tm * timeptr;
	struct timeval tv;

	curr_time = time(NULL);
	timeptr = localtime(&curr_time);
	gettimeofday(&tv, NULL);
	
	/* getting the hour, minutes, seconds format */
	long long int rc = strftime(ret, 20, "%H:%M:%S", timeptr);

	/* concatenating the milliseconds */
	char millisec[8];
	sprintf(millisec, ".%06ld", tv.tv_usec);
	strcat(ret, millisec);
	return ret;
}

/**
 * registers a log for the event performed 
 */
void add_event_log(char * me, char * event_type, char * packet_type, int seq, char * src, char * dest) {

	struct timeval tv;
	gettimeofday(&tv, NULL);
	char str[20];
	strcpy(str, get_local_time());
	long long int hr = (long long int) tv.tv_sec;
	hr *= 1000000;
	hr += tv.tv_usec;
	fprintf(event, FORMAT_SPEC_TEMP, me, event_type, hr, packet_type, seq, src, dest, str);
	printf(FORMAT_SPEC, me, event_type, str, packet_type, seq, src, dest);
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

/**
 * @param l 	The length of buffer written onto the file
 * the function shifts the recieved buffer to the left after write to a file 
 */
void shift_buffer_left(int l) {
	int i;
	for(i = 0; i < (WINDOW_SIZE + 1) * PACKET_SIZE - l; i++) {
		recieved_buffer[i] = recieved_buffer[i + l];
	}
	for(i = (WINDOW_SIZE + 1) * PACKET_SIZE - l; i < (WINDOW_SIZE + 1) * PACKET_SIZE; i++) {
		recieved_buffer[i] = '\0';
	}
}

/* Reporting error and exiting */
void die(char * s) {
	perror(s);
	exit(1);
}

/**
 * @param offset 	the sequence number of the acknowledgement packet
 * @param channel 	channel nunber through which ack will be sent
 * @param is_last 	whether the packet recieved is the last packet or not
 * returns a newly created Packet record with the specified fields
 */
Packet makePacket(int offset, int channel, int is_last) {

	Packet new;
	new.seq_no = offset;
	new.is_odd = channel;
	new.tag = 1;
	new.size = 0;
	new.is_last = is_last;
	return new;
}

int main() {

	/* initializing the event file pointer */
	event = fopen("event_server.txt", "w");
	printf(FORMAT_SPEC_HEAD, "Node Name", "Event Type", "Timestamp", "Packet Type", "Seq. No", "Source", "Dest");

	/* declarations */
	struct sockaddr_in si_me1, si_me2, si_relay;
	int slen_me1, slen_me2, slen_relay = sizeof(si_relay);
	int sock_odd, sock_even;
	int fd, recieved_last = 0, last_seq_no;
	offset_recieved = 0;
	memset(recieved_buffer, '\0', (WINDOW_SIZE + 1) * PACKET_SIZE);

	/* creating server socket */
	if((sock_odd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		die("Server odd socket not created.\n");
	}
	if((sock_even = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		die("Server even socket not created.\n");
	}

	/* populating the address of the server */
	memset((char *) &si_me1, 0, sizeof(si_me1));
	si_me1.sin_family = AF_INET;
	si_me1.sin_port = htons(PORTSERVER1);
	si_me1.sin_addr.s_addr = htonl(INADDR_ANY);
	slen_me1 = sizeof(si_me1);

	memset((char *) &si_me2, 0, sizeof(si_me2));
	si_me2.sin_family = AF_INET;
	si_me2.sin_port = htons(PORTSERVER2);
	si_me2.sin_addr.s_addr = htonl(INADDR_ANY);
	slen_me2 = sizeof(si_me2);

	/* binding the socket to the address */
	if(bind(sock_odd, (struct sockaddr *) &si_me1, sizeof(si_me1)) == -1) {
		die("Issue in binding 1\n");
	}
	if(bind(sock_even, (struct sockaddr *) &si_me2, sizeof(si_me2)) == -1) {
		die("Issue in binding 2\n");
	}

	fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);

	while(1) {

		/*initializing an fd_set */
		fd_set readfds;
		FD_ZERO(&readfds);

		/* insertinng the socket descriptor to the fd_set */
		FD_SET(sock_odd, &readfds);
		FD_SET(sock_even, &readfds);

		/* calling select for server */
		int ret = select(sock_even + 1, &readfds, NULL, NULL, NULL);

		if(ret == -1) {
			/* error occured in the select call */
			die("Error in select\n");
		}
		else if(ret) {
			/* niether error nor timeout occurred */
			int bytes_recieved, bytes_sent;
			Packet recieved_packet;

			if(FD_ISSET(sock_odd, &readfds)) {
				bytes_recieved = recvfrom(sock_odd, &recieved_packet, sizeof(recieved_packet), 0, (struct sockaddr *) &si_relay, &slen_relay);
			}
			else {
				bytes_recieved = recvfrom(sock_even, &recieved_packet, sizeof(recieved_packet), 0, (struct sockaddr *) &si_relay, &slen_relay);	
			}
			if(bytes_recieved == -1) {
				die("Error in recieve from relay\n");
			}

			if(recieved_packet.is_odd) 
				add_event_log("SERVER", "R", "DATA", recieved_packet.seq_no, "RELAY1", "SERVER");
			else 
				add_event_log("SERVER", "R", "DATA", recieved_packet.seq_no, "RELAY2", "SERVER");
			// printf("RCVD PKT: Seq. No %d of size %d Bytes from channel %d\n", 
			// 	recieved_packet.seq_no, recieved_packet.size, recieved_packet.is_odd);

			if(recieved_packet.is_last) {
				recieved_last = 1;
				last_seq_no = recieved_packet.seq_no;
			}

			copytobuffer(recieved_packet.seq_no, recieved_packet.payload);

			Packet ack = makePacket(recieved_packet.seq_no, recieved_packet.is_odd, recieved_packet.is_last);

			if(FD_ISSET(sock_odd, &readfds)) {
				bytes_sent = sendto(sock_odd, &ack, sizeof(ack), 0, (struct sockaddr *) &si_relay, slen_relay);
			}
			else {
				bytes_sent = sendto(sock_even, &ack, sizeof(ack), 0, (struct sockaddr *) &si_relay, slen_relay);
			}
			if(bytes_sent == -1) {
				die("Error in sending ack to relay\n");
			}

			if(ack.is_odd) 
				add_event_log("SERVER", "S", "ACK", recieved_packet.seq_no, "SERVER", "RELAY1");
			else 
				add_event_log("SERVER", "S", "ACK", recieved_packet.seq_no, "SERVER", "RELAY2");
			// printf("SENT ACK: for PKT with Seq. No. %d from channel %d\n", 
			// 	ack.seq_no, ack.is_odd);

			if(recieved_packet.seq_no == offset_recieved) {
				int tmp = write(fd, recieved_buffer, strlen(recieved_buffer));
				offset_recieved += strlen(recieved_buffer);
				// memset(recieved_buffer, '\0', strlen(recieved_buffer));
				shift_buffer_left(strlen(recieved_buffer));
				if(recieved_last && offset_recieved >= last_seq_no) {
					break;
				}
			}
		}
	}
	printf("\n");
	fclose(event);
	close(sock_odd);
	close(sock_even);
}