/************************************************
Author:
Name	Puneet Anand
ID 		2016B4A70487P
************************************************/

#include "packet.h"

/* This array will store the timeval structure of the send time of the current window */
struct timeval window_sendtime[WINDOW_SIZE];

/* This array will store the offsets of the packets sent in the current window */
int window_offset[WINDOW_SIZE];

/* This array will store the acknowledgement recieved flag for the packets sent in the current window */
int ack_recieved[WINDOW_SIZE];

/* This array will store the packets sent in the current window */
Packet window_packets[WINDOW_SIZE];

/* This array is a utility array for the current window */
int done[WINDOW_SIZE];

/* File pointer for storing the logs corresponding to client */
FILE * event;

/**
 * @param new 	new node that needs to be inserted to the log list
 * Inserts a new log record in the log list in sorted order 
 */
void insert_node(Node * new) {

	Node * curr, * prev;
	curr = head;
	prev = NULL;
	while(curr != NULL) {
		if(curr -> key > new -> key) {
			if(prev == NULL) {
				head = new;
				new -> next = curr;
			}
			else {
				prev -> next = new;
				new -> next = curr;
			}
			break;
		}
		prev = curr;
		curr = curr -> next;
	}
	if(curr == NULL) {
		if(head == NULL) {
			head = new;
			new -> next = NULL;
		}
		else {
			prev -> next = new;
			new -> next = NULL;
		}
	}
}

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
 * This function reads the 3 temporary log files and populate the linked list in sorted order for better analysis 
 */
void populate_list() {
	head = NULL;
	FILE * tmp = fopen("event_client.txt", "r");
	char buff[50];
	while(fscanf(tmp, "%s", buff) != EOF) {
		Node * new = (Node *)malloc(sizeof(Node));
		strcpy(new -> me, buff);
		fscanf(tmp, "%s", new -> event_type);
		fscanf(tmp, "%lld", &(new -> key));
		fscanf(tmp, "%s", new -> packet_type);
		fscanf(tmp, "%d", &(new -> seq));
		fscanf(tmp, "%s", new -> src);
		fscanf(tmp, "%s", new -> dest);
		fscanf(tmp, "%s", new -> currtime);
		insert_node(new);
	}
	tmp = fopen("event_relay.txt", "r");
	while(fscanf(tmp, "%s", buff) != EOF) {
		Node * new = (Node *)malloc(sizeof(Node));
		strcpy(new -> me, buff);
		fscanf(tmp, "%s", new -> event_type);
		fscanf(tmp, "%lld", &(new -> key));
		fscanf(tmp, "%s", new -> packet_type);
		fscanf(tmp, "%d", &(new -> seq));
		fscanf(tmp, "%s", new -> src);
		fscanf(tmp, "%s", new -> dest);
		fscanf(tmp, "%s", new -> currtime);
		insert_node(new);
	}
	tmp = fopen("event_server.txt", "r");
	while(fscanf(tmp, "%s", buff) != EOF) {
		Node * new = (Node *)malloc(sizeof(Node));
		strcpy(new -> me, buff);
		fscanf(tmp, "%s", new -> event_type);
		fscanf(tmp, "%lld", &(new -> key));
		fscanf(tmp, "%s", new -> packet_type);
		fscanf(tmp, "%d", &(new -> seq));
		fscanf(tmp, "%s", new -> src);
		fscanf(tmp, "%s", new -> dest);
		fscanf(tmp, "%s", new -> currtime);
		insert_node(new);
	}
}

/**
 * This function prints the list of logs in the file 
 */
void print_list() {

	Node * curr = head;
	FILE * tmp = fopen("logs_sorted.txt", "w");
	fprintf(tmp, FORMAT_SPEC_HEAD, "Node Name", "Event Type", "Timestamp", "Packet Type", "Seq. No", "Source", "Dest");
	while(curr != NULL) {
		fprintf(tmp, FORMAT_SPEC, curr -> me, curr -> event_type, curr -> currtime, curr -> packet_type, curr -> seq, curr -> src, curr -> dest);
		curr = curr -> next;
	}
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
 * @param offset	the offset of the data to be read from the file
 * @param channel	the channel through which the packet will be sent
 * @param fd 		the file descriptor of the file that is being uploaded
 * makes and returns a new packet with the following fields. 
 */
Packet makePacket(int * offset, int is_odd, int fd) {

	Packet new;
	new.seq_no = *offset;
	new.is_odd = is_odd;
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
 * @param sec 	Number of seconds before timeout
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
 * @param t 	struct timeval record for current time
 * @param new 	offset of the packet newly inserted in the window 
 * @param p 	newly send packet
 * Updates the timeval, offset, packets array containing of sendtimes offsets, packet records of current window
 */
void update_window_arrays(struct timeval t, int new, Packet p) {
	
	int i;
	for(i = 0; i < WINDOW_SIZE - 1; i++) {
		window_sendtime[i] = window_sendtime[i + 1];
		window_offset[i] = window_offset[i + 1];
		ack_recieved[i] = ack_recieved[i + 1];
		window_packets[i] = window_packets[i + 1];
		done[i] = done[i + 1];
	}
	window_sendtime[WINDOW_SIZE - 1] = t;
	window_offset[WINDOW_SIZE - 1] = new;
	ack_recieved[WINDOW_SIZE - 1] = 0;
	window_packets[WINDOW_SIZE - 1] = p;
	done[WINDOW_SIZE - 1] = 0;
}

/* reporting error and exiting */
void die(char * s) {
	perror(s);
	exit(1);
}

int main() {

	event = fopen("event_client.txt", "w");
	printf(FORMAT_SPEC_HEAD, "Node Name", "Event Type", "Timestamp", "Packet Type", "Seq. No", "Source", "Dest");
	/* declarations */
	int sock_odd, sock_even, conn_odd, conn_even;
	struct sockaddr_in si_r1, si_r2;
	int slen_r1, slen_r2;
	int fd, offset = 0;
	int i, j, k;
	int done = 0, sent_last = 0;

	/* opening sockets for odd and even packet transmission */
	if((sock_odd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		die("Socket odd not created\n");
	}
	if((sock_even = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		die("Socket even not created\n");
	}

	/* initializing addresses for relay R1 and R2 */
	memset((char *) &si_r1, 0, sizeof(si_r1));
	si_r1.sin_family = AF_INET;
	si_r1.sin_port = htons(PORTR1);
	si_r1.sin_addr.s_addr = inet_addr("127.0.0.1");
	slen_r1 = sizeof(si_r1);

	memset((char *) &si_r2, 0, sizeof(si_r2));
	si_r2.sin_family = AF_INET;
	si_r2.sin_port = htons(PORTR2);
	si_r2.sin_addr.s_addr = inet_addr("127.0.0.1");	
	slen_r2 = sizeof(si_r2);

	/* opening the file descriptor for the file to be transferred */
	fd = open("input.txt", O_RDWR, 0777);
	
	for(i = 0; i < WINDOW_SIZE; i++) {
		Packet new = makePacket(&offset, i % 2, fd);
		int tmp;
		if(i % 2) {
			tmp = sendto(sock_odd, &new, sizeof(Packet), 0, (struct sockaddr *) &si_r1, slen_r1);
		}
		else {
			tmp = sendto(sock_even, &new, sizeof(Packet), 0, (struct sockaddr *) &si_r2, slen_r2);
		}
		if(tmp == -1) {
			die("Problem in send\n");
		}

		if(i % 2)
			add_event_log("CLIENT", "S", "DATA", new.seq_no, "CLIENT", "RELAY1");
		else
			add_event_log("CLIENT", "S", "DATA", new.seq_no, "CLIENT", "RELAY2");
		// printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
		// 	new.seq_no, new.size, new.is_odd);
		gettimeofday(window_sendtime + i, NULL);
		window_offset[i] = new.seq_no;
		window_packets[i] = new;
		ack_recieved[i] = 0;
		if(new.is_last) {
			sent_last = 1;
			break;
		}
	}

	while(1) {

		if(done)
			break;

		/* contructing the timeval structure corresponding to timeout */
		struct timeval curr_time;
		gettimeofday(&curr_time, NULL);
		long long int seconds = (curr_time.tv_sec - window_sendtime[0].tv_sec);
		long long int micros = ((seconds * 1000000) + curr_time.tv_usec) - (window_sendtime[0].tv_usec);
		micros = RETRANS_TIME * 1000000 - micros;
		if(micros <= 0)
			micros = 0;
		struct timeval timeout = makeTimeVal(micros);

		/*initializing an fd_set */
		fd_set readfds;
		FD_ZERO(&readfds);

		/* insertinng the 2 socket descriptors to the fd_set */
		FD_SET(sock_odd, &readfds);
		FD_SET(sock_even, &readfds);

		/* Calling select and noting the time taken by the call */
		struct timeval start, end;
		gettimeofday(&start, NULL);
		int ret = select(sock_even + 1, &readfds, NULL, NULL, &timeout);
		gettimeofday(&end, NULL);

		if(ret == -1) {
			/* error occured in the select call */
			die("Error in select\n");
		}
		else if(ret) {
			/* niether error nor timeout occured */
			Packet recieved_packet;
			int bytes_recieved, bytes_sent;
			if(FD_ISSET(sock_odd, &readfds)) {
				bytes_recieved = recvfrom(sock_odd, &recieved_packet, sizeof(recieved_packet), 0, (struct sockaddr *) &si_r1, &slen_r1);
			}
			else {
				bytes_recieved = recvfrom(sock_even, &recieved_packet, sizeof(recieved_packet), 0, (struct sockaddr *) &si_r2, &slen_r2);
			}
			if(bytes_recieved == -1) {
				die("Error in recieve\n");
			}

			if(recieved_packet.is_odd)
				add_event_log("CLIENT", "R", "ACK", recieved_packet.seq_no, "RELAY1", "CLIENT");
			else
				add_event_log("CLIENT", "R", "ACK", recieved_packet.seq_no, "RELAY2", "CLIENT");
			// printf("RCVD ACK: for PKT with Seq. No. %d from channel %d\n", 
			// 	recieved_packet.seq_no, recieved_packet.is_odd);
			
			/* calculating the index of the recieved packet wrt window start */
			int idx = (recieved_packet.seq_no - window_offset[0]) / PACKET_SIZE;
			if((recieved_packet.seq_no - window_offset[0]) % PACKET_SIZE)
				idx++;

			ack_recieved[idx] = 1;
			
			/* shifting the window as much as possible */
			while(ack_recieved[0] && done == 0) {
				Packet new = makePacket(&offset, (offset / PACKET_SIZE) % 2, fd);
				if(window_packets[0].is_last) {
					done = 1;
				}
				
				update_window_arrays(end, new.seq_no, new);
				
				if(new.is_last && sent_last) 
					continue;
				else if(new.is_last)
					sent_last = 1;
				if(new.is_odd) {
					bytes_sent = sendto(sock_odd, &new, sizeof(Packet), 0, (struct sockaddr *) &si_r1, slen_r1);
				}
				else {
					bytes_sent = sendto(sock_even, &new, sizeof(Packet), 0, (struct sockaddr *) &si_r2, slen_r2);
				}

				if(new.is_odd)
					add_event_log("CLIENT", "S", "DATA", new.seq_no, "CLIENT", "RELAY1");
				else
					add_event_log("CLIENT", "S", "DATA", new.seq_no, "CLIENT", "RELAY2");
				// printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
				// 	new.seq_no, new.size, new.is_odd);
			}
		}
		else {
			/* timeout occurred */
			// printf("Timeout for offset %d\n", window_packets[0].seq_no);
			int bytes_sent;
			Packet new = window_packets[0];
			if(window_packets[0].is_odd) {
				bytes_sent = sendto(sock_odd, &new, sizeof(Packet), 0, (struct sockaddr *) &si_r1, slen_r1);
			}
			else {
				bytes_sent = sendto(sock_even, &new, sizeof(Packet), 0, (struct sockaddr *) &si_r2, slen_r2);	
			}

			if(new.is_odd)
				add_event_log("CLIENT", "S", "DATA", new.seq_no, "CLIENT", "RELAY1");
			else
				add_event_log("CLIENT", "S", "DATA", new.seq_no, "CLIENT", "RELAY2");
			// printf("SENT PKT: Seq. No %d of size %d Bytes from channel %d\n", 
			// 	new.seq_no, new.size, new.is_odd);
			window_sendtime[0] = end;
		}
	}
	/* closing the sockets */
	close(sock_odd);
	close(sock_even);
	
	/* closing the temporary file descriptor */
	fclose(event);

	/* printing the logs to the file 'logs_sorted.txt' */
	head = NULL;
	populate_list();
	print_list();

	printf("\n");
	/* removing the temporary files created */
	remove("event_client.txt");
	remove("event_relay.txt");
	remove("event_server.txt");	
}