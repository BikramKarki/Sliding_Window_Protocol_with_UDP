// Referenced from EchoServer.c
// Bikram Karki
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Define a constant for the maximum number of packets
#define ECHOMAX 347

// Function definition for error handling
void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}


// Structure to represent an acknowledgment packet
struct ack_pkt_t

{
    int type;
    int ack_no;
};

// Main function
int main(int argc, char *argv[])
{
    // Variable declarations
    int sock; /*Socket Descriptor*/
    struct sockaddr_in echoServAddr;//Local address
    struct sockaddr_in echoClntAddr;//Client address
    unsigned int cliAddrLen;//length of incoming message
    unsigned short echoServPort;//server port
   

    // Check for correct number of command-line arguments
    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s <len> <port_no> <a_list_of_numbers>\n", argv[0]);
        exit(1);
    }

    // Parse command-line arguments
    int len = atoi(argv[1]);//
    echoServPort = atoi(argv[2]);

    // Initialize array to hold the list of packets to be dropped
    int dropList[argc - 3];
    for (int i = 0; i < argc - 3; i++)
    {
        dropList[i] = atoi(argv[i + 3]);
    }
    int dropListSize = argc - 3;
    // Create a UDP socket
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    // Initialize the server address structure
    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    echoServAddr.sin_port = htons(echoServPort);

    // Bind the socket to the specified port
    if (bind(sock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");

    // Initialize the receiving buffer and an array to keep track of received packets
    char receivingBuffer[ECHOMAX];
    int packet_rcvd[ECHOMAX] = {0};

    // Infinite loop to continuously receive packets from the client
    while(1)
    {
        cliAddrLen = sizeof(echoClntAddr);

        // Structure to represent a data packet
        struct data_pkt_t
        {
            int type;
            int seq_no;
            int length;
            char data[1024];
        } receive_dp;

        int rlen = 3 * 4 + len;

        // Receive a data packet from the client
        if (recvfrom(sock, &receive_dp, rlen, 0, (struct sockaddr *)&echoClntAddr, &cliAddrLen) < 0)
            DieWithError("recvfrom() failed");

        // Print the received sequence number
        printf("RECEIVE Packet %d\n", receive_dp.seq_no);

        // Check if the received packet should be dropped
        int shouldDrop = 0;
        for (int i = 0; i < dropListSize; i++)
        {
            if (receive_dp.seq_no == dropList[i])
            {
                shouldDrop = 1;
                printf("---------- DROP Packet %d\n", receive_dp.seq_no);
                dropList[i] = -1;
                break;
            }
        }

        // If the packet should not be dropped
        if (!shouldDrop)
        {
            // Update the receiving buffer and the array tracking received packets
            packet_rcvd[receive_dp.seq_no] = 1;
            memcpy(receivingBuffer + receive_dp.seq_no * len, receive_dp.data, receive_dp.length);

            // Find the highest continuous sequence number acknowledged
            int ack_no = 0;
            for (int i = 0; i < ECHOMAX; i++)
            {
                if (packet_rcvd[i])
                {
                    ack_no = i;
                }
                else
                {
                    break;
                }
            }

            // Create an ACK packet for the highest continuous sequence number
            struct ack_pkt_t ack;
            ack.type = 2;
            ack.ack_no = ack_no;

            // Send the ACK packet back to the client
            if (sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&echoClntAddr, cliAddrLen) != sizeof(ack))
                DieWithError("sendto() sent a different number of bytes than expected");

            // Print the sent ACK number
            printf("------ SEND ACK %d\n", ack.ack_no);

            // If the highest continuous sequence number acknowledged is equal to the number of packets,
            // it means the server has received all packets
            if (ack.ack_no == (ECHOMAX+ len -1)/len-1)
            {
                printf("Received message: %s\n", receivingBuffer);
                // Save the received message to a file
                FILE *outFilePtr;
                outFilePtr = fopen("receivedFile.txt", "wb");
                

                if (fwrite(receivingBuffer, 1, sizeof(receivingBuffer), outFilePtr) != sizeof(receivingBuffer) || outFilePtr == NULL)
                {
                    printf("Error writing to file!\n");
                }
                fclose(outFilePtr);

                break; // Exit the loop after saving the received data
            }
        }
    }

}

