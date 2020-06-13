# Computer-Networks-Assignment

### Multi-channel stop-and-wait protocol without DATA or ACK losses.

1. The sender transmits packets through two different channels (TCP connections).
2. The server acknowledges the receipt of a packet via the same channel through which the
corresponding packet has been received.
3. The sender transmits a new packet using the same channel on which it has received an ACK for its
one of the previously transmitted packet. Note that, at a time, there can be at most two outstanding
unacknowledged packets at the sender side.
4. On the server-side, the packets transmitted through different channels may arrive out of order. In
that case, the server has to buffer the packets temporarily to finally construct in-order data stream.


### Selective Repeat over UDP
[Insert the topology image here] 

![alt text](https://github.com/adam-p/markdown-here/raw/master/src/common/images/icon48.png "Logo Title Text 1")

C uploads input.txt to S. All odd-numbered packets go through the relay node R1, while all even-
numbered packets go through the relay node R2. R1 and R2 add a delay, which is a random number

distributed uniformly between 0 to 2 ms for a given packet. Acknowledgments can take any route and
do not suffer from delays or packet drops.
