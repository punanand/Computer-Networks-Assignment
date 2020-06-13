
### Files:

* client.c
* server.c
* packet.h
* readme.txt
* input.txt (File to be uploaded)
* client (executable binary)
* server (executable binary)

### Execution Commands:

Open terminal 1:

Step 1: `$ gcc -g server.c -o server`
Step 2: `$ ./server`

Open terminal 2:

Step 3: `$ gcc -g client.c -o client`
Step 4: `$ ./client`

_(NOTE: Commands are to be executed in the given order)_

File to be uploaded by the client: `input.txt`
Uploaded file after execution at the server's end: `output.txt`

Server and client traces will be printed on their respective terminals.

### Specification:  			

The modification in the specifications can be done in the file 'packet.h'.

1. `PACKET_SIZE` (Default: 100)
   Number of bytes of payload contained in one packet 		
2. `PDR` (Default: 10)
   Packet Drop Rate, the probability at which server drops a received packet (in %)
3. `RETRANS_TIME` (Default: 2)
   Packet retransmission time, time in seconds after which a given packet will be resent if acknowledgement is not received from the server
4. `BUFFER_SIZE` (Default: 5) *(NOTE: Keep this buffer size atleast 2)*
   The character buffer size as a multiple of packet size, since it is assumed to be approximately one tenth of the input size, this may be changed according to packet size and input file size.
   
### Assumptions:

1. Assumed that the size of the character buffer available at the server's side is `BUFFER_SIZE` (defined as macro) * `PACKET_SIZE`.
2. Any packet received if the buffer is full and can't be written to the file will be dropped by the server.
3. Since the packets may be dropped due to limited buffer size, so even if `PDR` is 0 packets may be dropped.
4. Package reception log is not printed if the packet is dropped at the server's end.

### Implementation:

#### Client: 
1. Opening 2 sockets and connecting them to the server.
2. Initially sending 2 packets through each of the channels, if there are enough bytes to be sent, otherwise, 1 packet may suffice.
3. Maintaining 2 timeval structures, each for storing the time of the last send from the respective channels.
4. Initialize a `fd_set` in each iteration consisting of the 2 socket descriptors.
5. Using select on the `fd_set` created, monitor if any acknowledgements are received from the server with the timeout structure dependent on the channel which sent the packet earlier, also calculate the time taken by the call.
6. If an acknowledgement is received, then make a new packet and send it to the server, also update the timeval structure for the timeout and the 2 channels using the time taken by `select()` routine to return.
7. If a timeout happens, it is easy to know that the channel through which the last packet sent was earlier caused the timeout, resend the packet which was stored for the corresponding channel and update the relevant timeval structures.
8. Begin a new iteration.
9. Stopping condition: whenever we receive ack for the last packet (can be known by `is_last` field in the Packet structure), break the loop and close the sockets.

#### Server:
1. Open a socket, bind to the port, populate the `sockaddr_in` structure.
2. Accepting the connection request from the 2 connections, one for each channel (it will give us 2 socket descriptors).
3. Initialize an `fd_set` in each iteration consisting of the 2 socket descriptors.
4. Using select on the fd_set created, monitor if any data packets are received from the client with the timeout structure being `NULL`.
5. Receive the packet 
6. Generate a random number, if it is less than `PDR`, drop the packet.
7. If the buffer size is full and the packet received doesn.t have the desired sequence number, drop the packet.
8. Otherwise, copy the payload to the buffer and send an ack packet through the channel from which the packet was received.
6. Buffering: 
    1. Initialize the expected offset to 0.
    2. Whenever a packet is received, check whether it's sequence number differs from the expected sequence number by less than the buffer size.
    3. If the condition holds, the buffer has enough space to accommodate the received payload.
    4. Catch: Since it is a stop and wait protocol and packets are created after an ack is received, only 1 packet can be out of order, and the remaining payloads will be continuous.
    5. If the received packet has the expected sequence number, empty the buffer and update the expected sequence number.
7. Go to the next iteration
8. Stopping condition: Whenever we empty the buffer after properly receiving the last packet, we break the loop. 
9. `output.txt` uploaded on the server's side.
