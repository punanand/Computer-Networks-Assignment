/************************************************
Author:
Name	Puneet Anand
ID 		2016B4A70487P
************************************************/

#include "packet.h"

/* This array stores the microseconds left for the packet to be sent to the server */
long long int time_left[WINDOW_SIZE];

/* This array stores the flag indicating the validity of the entry in window */
int valid[WINDOW_SIZE];

/* This array stores packets to be sent to the server */
Packet recieved_packets[WINDOW_SIZE];

/* File pointer for storing the logs corresponding to relay */
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
	fflush(event);
	printf(FORMAT_SPEC, me, event_type, str, packet_type, seq, src, dest);
}

/**
 * This function returns an index containing non valid entry in the array for storing the newly recieved data 
 */
int get_empty_index() {
	
	int i;
	for(i = 0; i < WINDOW_SIZE; i++)
		if(valid[i] == 0)
			return i;
	return -1;
}

/**
 * @param usec  Number of micro-seconds before timeout
 * returns a timeval structure containing of 
 */
struct timeval makeTimeVal(long long int tot_usec) {

	/* initializing the timeval structure */
	struct timeval timeout;
	timeout.tv_sec = tot_usec / 1000000;
	timeout.tv_usec = tot_usec % 1000000;
	return timeout;
}

/**
 * Returns the timeval record corresponding to the packet whose time left to send the data is minimum
 */
struct timeval get_min_sendtime(int * min_idx) {

	int i;
	long long int mn = 100000000000;
	for(i = 0; i < WINDOW_SIZE; i++) {
		if(valid[i] && time_left[i] < mn) {
			mn = time_left[i];
			*min_idx = i;
		}
	}
	return makeTimeVal(mn);
}

/**
 * @param start 	It is the timeval record before the call to select
 * @param end 		It is the timeval record after the call to select
 * This function updates the time left array after the call to select 
 */
void update_time_array(struct timeval start, struct timeval end) {
	
	long long int seconds = (end.tv_sec - start.tv_sec);
	long long int micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
	int i;
	for(i = 0; i < WINDOW_SIZE; i++) {
		if(valid[i]) {
			time_left[i] -= micros;
			if(time_left[i] < 0)
				time_left[i] = 0;
		}
	}
}

/* reporting error and exiting */
void die(char * s) {
	perror(s);
	exit(1);
}

/**
 * Pass the relay number as argument
 * "1" for Relay 1
 * "2" for Relay 2
 */
int main(int argc, char ** argv) {

	event = fopen("event_relay.txt", "a");
	printf(FORMAT_SPEC_HEAD, "Node Name", "Event Type", "Timestamp", "Packet Type", "Seq. No", "Source", "Dest");
	srand(time(0));

	/* declarations */
	struct sockaddr_in si_me, si_client, si_server1, si_server2, si_tmp;
	int slen_me, slen_client = sizeof(si_client), slen_server1, slen_server2, slen_tmp = sizeof(si_tmp);
	int sock;
	int i, j, k;

	/* creating socket */
	if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		die("Socket not created\n");
	}

	/* populating the address of relay according to the arguments passed */
	memset((char *) &si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	if(strcmp(argv[1], "1") == 0)
		si_me.sin_port = htons(PORTR1);
	else
		si_me.sin_port = htons(PORTR2);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	slen_me = sizeof(si_me);

	/* populating the address for the server */
	memset((char *) &si_server1, 0, sizeof(si_server1));
	si_server1.sin_family = AF_INET;
	si_server1.sin_port = htons(PORTSERVER1);
	si_server1.sin_addr.s_addr = htonl(INADDR_ANY);
	slen_server1 = sizeof(si_server1);

	memset((char *) &si_server2, 0, sizeof(si_server2));
	si_server2.sin_family = AF_INET;
	si_server2.sin_port = htons(PORTSERVER2);
	si_server2.sin_addr.s_addr = htonl(INADDR_ANY);
	slen_server2 = sizeof(si_server2);	

	/* binding the socket to the port number */
	if(bind(sock, (struct sockaddr*) &si_me, sizeof(si_me)) == -1) {
		die("Problem in bind\n");
	}

	/* initializing the valid array */
	for(i = 0; i < WINDOW_SIZE; i++) 
		valid[i] = 0;

	while(1) {

		int min_idx;
		struct timeval timeout = get_min_sendtime(&min_idx);

		/*initializing an fd_set */
		fd_set readfds;
		FD_ZERO(&readfds);

		/* insertinng the socket descriptor to the fd_set */
		FD_SET(sock, &readfds);

		/* Calling select and noting the time taken by the call */
		struct timeval start, end;
		gettimeofday(&start, NULL);
		int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
		gettimeofday(&end, NULL);

		/* updating the time left */
		update_time_array(start, end);

		if(ret == -1) {
			/* error occured in the select call */
			die("Error in select\n");
		}
		else if(ret) {
			/* niether error nor timeout occurred */
			Packet recieved_packet;
			int bytes_recieved, bytes_sent;

			/* recieving the packet from client */
			bytes_recieved = recvfrom(sock, &recieved_packet, sizeof(recieved_packet), 0, (struct sockaddr *) &si_tmp, &slen_tmp);
			if(si_tmp.sin_port != si_server1.sin_port && si_tmp.sin_port != si_server2.sin_port) {
				/* packet is recieved from the server */
				si_client = si_tmp;
			}
			if(bytes_recieved == -1) {
				die("Error while recieving data from client or server.\n");
			}

			if(recieved_packet.tag == 0) {
				/* given packet is recieved from the client */

				/* estimating the delay in this packet in microseconds */
				int delay = (rand() % 2000);
				int idx = get_empty_index();
				time_left[idx] = delay;

				if(recieved_packet.is_odd)
					add_event_log("RELAY1", "R", "DATA", recieved_packet.seq_no, "CLIENT", "RELAY1");
				else
					add_event_log("RELAY2", "R", "DATA", recieved_packet.seq_no, "CLIENT", "RELAY2");
				// printf("RCVD PKT: Seq. No %d of size %d Bytes from channel %d\n", 
				// 	recieved_packet.seq_no, recieved_packet.size, recieved_packet.is_odd);

				/* estimating if the packet will be dropped or not */
				int drop = (rand() % 100);
				if(drop < PDR)
					continue;
				else {
					valid[idx] = 1;
					recieved_packets[idx] = recieved_packet;
				}
			}
			else {
				/* given packet is recieved from server */
				
				if(recieved_packet.is_odd)
					add_event_log("RELAY1", "R", "ACK", recieved_packet.seq_no, "SERVER", "RELAY1");
				else
					add_event_log("RELAY2", "R", "ACK", recieved_packet.seq_no, "SERVER", "RELAY2");
				// printf("RCVD ACK: for PKT with Seq. No. %d from channel %d\n", 
				// 	recieved_packet.seq_no, recieved_packet.is_odd);
				
				bytes_sent = sendto(sock, &recieved_packet, sizeof(recieved_packet), 0, (struct sockaddr *) &si_client, slen_client);
				if(bytes_sent == -1) {
					die("Error in sending back ack to the client\n");
				}

				if(recieved_packet.is_odd)
					add_event_log("RELAY1", "S", "ACK", recieved_packet.seq_no, "RELAY1", "CLIENT");
				else
					add_event_log("RELAY2", "S", "ACK", recieved_packet.seq_no, "RELAY2", "CLIENT");
				// printf("SENT ACK: for PKT with Seq. No. %d from channel %d\n", 
				// 	recieved_packet.seq_no, recieved_packet.is_odd);
			}			
		}
		else {
			/* timeout occured */
			/* time for some packet to be sent to the server */

			int bytes_sent, bytes_recieved;
			Packet new = recieved_packets[min_idx];

			/* sending the corresponding packet and recieving the ack back from the server */
			if(strcmp(argv[1], "1") == 0)
				bytes_sent = sendto(sock, &new, sizeof(Packet), 0, (struct sockaddr *) &si_server1, slen_server1);
			else
				bytes_sent = sendto(sock, &new, sizeof(Packet), 0, (struct sockaddr *) &si_server2, slen_server2);
			if(bytes_sent == -1) {
				die("Error in send\n");
			}

			if(new.is_odd)
				add_event_log("RELAY1", "S", "DATA", new.seq_no, "RELAY1", "SERVER");
			else
				add_event_log("RELAY2", "S", "DATA", new.seq_no, "RELAY2", "SERVER");
			// printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
			// 	new.seq_no, new.size, new.is_odd);

			valid[min_idx] = 0;
		}
	}
}