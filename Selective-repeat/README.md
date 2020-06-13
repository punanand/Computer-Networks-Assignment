## Selective-Repeat

### Files:

* client.c
* server.c
* relay.c
* packet.h
* readme.txt
* input.txt (File to be uploaded at the server's end)
* logs_sorted.txt (File created after the execution, contains information regarding logs)
* client (executable binary)
* server (executable binary)
* relay (executable binary)

### Execution Commands:

#### Open terminal 1: (Instantiate server)

Step 1: `$ gcc -g server.c -o server`
Step 2: `$ ./server`

#### Open terminal 2: (Instantiate Relay 1, "1" is passed as an arg)

Step 3: `$ gcc -g relay.c -o relay`
Step 4: `$ ./relay 1`

#### Open terminal 3: (Instantiate Relay 2, "2" is passed as an arg)

Step 5: `$ gcc -g relay.c -o relay`
Step 6: `$ ./relay 2`

#### Open terminal 4: (Instantiate client)

Step 7: `$ gcc -g client.c -o client`
Step 8: `$ ./client`

File to be uploaded by the client: `input.txt`
Uploaded file after execution at the server's end: `output.txt`

The logs in the required tabular format: `logs_sorted.txt`

Separate event logs are also printed on the respective terminals.

### Specification:  			

The modification in the specifications can be done in the file `packet.h`.

1. `PACKET_SIZE` (Default: 100)
   Number of bytes of payload contained in one packet 		
2. `PDR` (Default: 10)
   Packet Drop Rate, the probability at which server drops a received packet (in %)
3. `RETRANS_TIME` (Default: 2)
   Packet retransmission time, time in seconds after which a given packet will be resent if acknowledgement is not received from the server
4. `WINDOW_SIZE` (Default: 4)
   The size of the window for the selective repeat protocol.
5. The timestamp obtained in 

### Assumptions:

1. Assumed that the size of the character buffer available at the server's side is `WINDOW_SIZE * PACKET_SIZE` (it will be sufficient as the maximum number of un-acked packets in Selective Repeat protocol can be no more than window size).
2. If a packet is dropped by the Relay, it is not been logged.
3. The Relays are independent of the working of Client and Server, they just process the packets (add delays, may drop) and forwards them. So they may not stop their execution on their own. It needs to be done manually after the client and server program exits.
4. Sequence numbers chosen are offset in the file from where the corresponding payload is populated.


### Implementation:

#### Client: 
1. Following arrays of the same size as window are maintained (Let's collectively call them `window arrays`):
    1. window_sendtime: stores the time at which the packet corresponding to the index in the window was sent to the relay.
	_(NOTE: This will further help us implement a separate timer for each packet)_
	  2. window_offset: stores the sequence number of the packet corresponding to the index in the window.
	  3. ack_recieved: stores the flag whether ack of the packet corresponding to the index in the window is received by the client.
	_(NOTE: This will further help us shift the window)_
	  4. window_packets: stores the packets corresponding to the index in the window.
	(NOTE: This will help us resend the packet in case it is dropped by the delay, without creating it again)
2. Open 2 sockets (sock_odd and sock_even) and instantiated the sockaddr_in structures for Relay1 and Relay2.
3. Initially send the window size number of packets to the respective relay (depending on odd or even multiple of `PACKET_SIZE`), if there is enough data, otherwise send the packets that cover all the data, and populate the window arrays appropriately.
4. Calculate the time lapse between the current system time and the send time of the packet corresponding to index 0 (first packet of the window), time left to `timeout = Retransmission time - calculated time difference`.
5. Initiate a fd_set for the 2 sockets created for the relays.
6. Using select routine, monitor if any acknowledgements are received from any of the relays, or if the timeout occurs.
7. If acknowledgement received, iterate and check whether ack received for the corresponding packet at index 0, and slide the window and check again, if ack not received, break.
(NOTE: during the slide of the window, window arrays are shifted left)
8. If the timeout happens, the corresponding packet is resent through the respective relay and window arrays are updated.
9. Go to step 4.
10. Stopping condition: if ack is received for the packet at index 0 of the window (first packet of the window), and it is the last packet (checked using the is_last flag in the Packet structure).
11. After that logs are generated.

#### Relay:
1. Following arrays of the same size as the window is maintained (Let's collectively call them `relay arrays`):
(NOTE: Since in Selective Repeat protocol the number of un-acked packets can be at most the size of the window, the size of the array can be as that of the window)
    1. `time_left`: stores the time left for the packet to be forwarded to the server.
	(NOTE: Since relay gives a random delay to the packet, it updates the corresponding entry in this array)
	  2. `recieved_packets`: stores the packets to be forwarded to the server.
2. Open a socket and bind to the port number depending on the line argument passed to the executable.
3. Instantiate the `sockaddr_in` records for the 2 server ports.
4. Iterate over the `time_left` array and find the Packet that is to be forwarded to the server the earliest.
5. Initialize a `fd_set` with the opened socket and timeout corresponding to the time calculated in step 4.
6. Using select monitor any received packets at the socket or check if a timeout happens, also time the select call and update the `time_left` arrays by subtracting by the estimated time.
7. If a packet is received, it may be `ACK` packet or a `DATA` packet (it may be checked by looking at the tag field of the Packet structure).
8. If a data packet is received from the client, calculate the delay to be assigned and check whether it is to be dropped or not, if it not being dropped, corresponding relay arrays are populated with its entries.
9. If ack is received from the server, this ack is forwarded to the client without any delay.
10. If the timeout occurs, it implies that is time for some data packet to be forwarded to the server, the corresponding packet is found out and forwarded to the server.
11. Return to step 4.
_(NOTE: Relay routine goes on independent of the file size, so it will not have any stopping condition, it needs to be manually restarted for uploading another file)_

#### Server:
1. Open 2 sockets, one for Relay 1, other for Relay 2 and bind them to their corresponding ports.
2. Initialize an `fd_set` in each iteration consisting of the 2 sockets.
3. Using select on the `fd_set` created, monitor if any data packets are recieved from the client with the timeout structure being `NULL`.
4. Receive the packet.
5. Send an ack with the corresponding sequence number.
5. Add the payload to the buffer.
6. Buffering: 
    1. Initialize the expected offset to 0.
	  2. Whenever a packet is received, check whether it's sequence number differs from the expected sequence number by less than the buffer size.
	  3. If the condition holds, the buffer has enough space to accommodate the received payload.
	  4. If the received packet has the expected sequence number, empty the buffer and update the expected sequence number.
_(NOTE: taking buffer size to be WINDOW_SIZE * PACKET_SIZE, the buffer will always be sufficient, and no overflow conditions may arise in Selective Repeat protocol)_
7. Go to the next iteration
8. Stopping condition: Whenever we empty the buffer after properly receiving the last packet, we break the loop. 
9. `output.txt` uploaded on the server's side.

Logging the events (sorted with respect to timestamp):
1. Each of the application (client, server, Relay 1, Relay 2) write their logs to a temporary file while their execution.
2. After the client breaks out of the loop, it reads these files in does online insertion sort on these logs after adding them to a node structure.
3. Finally, the logs are output on `logs_sorted.txt`
