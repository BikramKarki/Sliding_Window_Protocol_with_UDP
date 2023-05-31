# UDP-with-Sliding-Window

Author: Bikram Karki

## Description  
This program implements the sliding window protocol in UDP by transferring a string. The maximum length of data is 350 from the buﬀer. The total number of packets that need to be received successfully at the receiver depends on the len provided in the argument. Their sequence numbers are 0, 1, 2, · · · Buffer size/len. The two are two programs `sender.c` and `receiver.c` if run in two terminals within a same local machine (127.0.0.1) take part in the communication with sliding window protocol.  To simulate the loss of packets a sequence of packets to be dropped is passed as argument while running the server program.

## Compilation  
makefile is provided which can be run using command `$ make` in the working directory which contains the program for sender and receiver.

## Usage
After compliation two executables are available in the directory which can be run with the usage:  
`$ ./myclient <len> <server_ip> <server_port>`  
for example: ./myclient 10 127.0.0.1 5000 
`$ ./myserver <len> <server_port> <List of numbers passed as command line argument to simulate the packet loss>`
for example: ./myserver 10 5000 1 5 9

## Implementation Details
The client reads server ip, server port. The string to be sent is already initialized within the client program. The total size of the UDP message
is 4*3+length bytes.The size of messages client will sent is sizeof(int)*3 + length. The total number of packets that need to be sent out is N = ⌈350/len⌉. For example if the len provided from the argument is 10. Then, the total number of packets is 35. Their sequence numbers are 0, 1, 2, · · · , N − 1. The ACK used is cumulative at the packet level. ACK n means all packets up to packet n have been received. The wait time for receieving ack is fixed at 4 secs.
The receiver declares a receiving buffer to hold the content sent from the sender. After receiving a packet from a sender, it will determine whether to drop the packet or not by checking with the list of numbers parameter provided as arguments when running the program. If the packet is not dropped cumulative ACK is sent back to the client.


