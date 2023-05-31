//It is the sender or the client for UDP
//Bikram Karki
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#define TIMEOUT_SECS 4
#define MAX_TRIES 5

typedef struct {
    int type;
    int seq_no;
    int length;
    char data[1024];
} data_pkt_t;

typedef struct {
    int type;
    int ack_no;
} ack_pkt_t;
/* Handler for SIGALRM */
void catch_alarm(int ignored) {
    return;
}
// Configure server address structure
void initialize_server_address(struct sockaddr_in *server_addr, char *server_ip, unsigned short server_port) {
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = inet_addr(server_ip);
    server_addr->sin_port = htons(server_port);
}
// Set up signal handling for timeout
void setup_signal_handling() {
    struct sigaction myAction;
    myAction.sa_handler = catch_alarm;

    if (sigfillset(&myAction.sa_mask) < 0) {
        perror("sigfillset() failed");
        exit(1);
    }

    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0) {
        perror("sigaction() failed");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 4) {
        fprintf(stderr, "Usage: %s len server_IP server_port\n", argv[0]);
        exit(1);
    }

    int len = atoi(argv[1]);
    char *server_ip = argv[2];
    unsigned short server_port = atoi(argv[3]);
// Pre-defined buffer
    char buffer[350] = "The University of Kentucky is a public, land grant university dedicated to improving people's lives through excellence in education,"
                       " research and creative work, service and health care. As Kentucky's flagship institution, the University plays a critical leadership role by promoting"
                       " diversity, inclusion, economic development and human well-being.";
    
    // Calculate total number of packets
    int N = (strlen(buffer) + len - 1) / len;

    int sock_fd;
    // Create UDP socket
    if ((sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket() failed");
        exit(1);
    }

    struct sockaddr_in server_addr;
    initialize_server_address(&server_addr, server_ip, server_port);

    setup_signal_handling();

    
    data_pkt_t data_pkt;
    ack_pkt_t ack_pkt;
    struct sockaddr_in from_addr;
    unsigned int from_size;
    int base = 0;
    int last_sent = 5;
    int ndups = 0;
    int tries = 0;
    //Send the first packets with window size 6 
    for (int i = 0; i < 6; i++) {
        data_pkt.type = 1;
        data_pkt.seq_no = i;
        data_pkt.length = (i < N - 1) ? len : strlen(buffer) - (N - 1) * len;
        memcpy(data_pkt.data, buffer + i * len, data_pkt.length);

        sendto(sock_fd, &data_pkt, sizeof(int) * 3 + data_pkt.length, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        printf("SEND PACKET %d\n", data_pkt.seq_no);
    }

    alarm(TIMEOUT_SECS);
    //The loop for receiving Acks
    while (1) {
         // Receive ACKs from the reciever and store the address
        from_size = sizeof(from_addr);
        ssize_t resp_len = recvfrom(sock_fd, &ack_pkt, sizeof(ack_pkt_t), 0, (struct sockaddr *)&from_addr, &from_size);

        if (resp_len < 0) {
            //for the limit of maximum number of tries
            if (tries >= MAX_TRIES) {
                perror("The maximum tries have crossed the limit");
                exit(1);
            }

            tries++;
            printf("TIMEOUT. Resend window!\n");
            printf("%d more tries ...\n", MAX_TRIES - tries);
        //Retransmitting the packets
            for (int i = base; i <= last_sent; i++) {
                data_pkt.type = 1;
                data_pkt.seq_no = i;
                data_pkt.length = (i < N - 1) ? len : strlen(buffer) - (N - 1) * len;
                memcpy(data_pkt.data, buffer + i * len, data_pkt.length);
            //Sending the packet to the server
                sendto(sock_fd, &data_pkt, sizeof(int) * 3 + data_pkt.length, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                printf("RESEND PACKET %d\n", data_pkt.seq_no);
            }
            //Alarm Reset
            alarm(TIMEOUT_SECS);
        } else {
            //ACK Received
            tries = 0;
            alarm(0);//clears the timer
            printf("------ RECEIVE ACK %d\n", ack_pkt.ack_no);
            // Checking if the received ACK matches the last packet
            // If the ack number (ack_no in the received ACK ) is N− 1, the sender is done with the process and exits
            if (ack_pkt.ack_no == N - 1) {
                break;
            } 
    
            // If the ack number is equal to or greater than base, it is a new ACK. In this case, set ndups
            // to 0, update base to be equal to the ack number + 1, and send new packets allowed by the
            // window size, i.e., packets with sequence number from last_sent+1 until the sequence
            // number min(base+5, N-1); update the last_sent variable to min(base+5, N-1).
            else if (ack_pkt.ack_no >= base) {
                //Resetting the number of duplicate acks to zero and updating the base
                ndups = 0;
                base = ack_pkt.ack_no + 1;
                //Checking the data packet to send
                while (last_sent < base + 5 && last_sent < N - 1) {
                    last_sent++;
                    data_pkt.type = 1;
                    data_pkt.seq_no = last_sent;
                    data_pkt.length = (last_sent < N - 1) ? len : strlen(buffer) - (N - 1) * len;
                    memcpy(data_pkt.data, buffer + last_sent * len, data_pkt.length);
                    //Sending the data packet to the server
                    sendto(sock_fd, &data_pkt, sizeof(int) * 3 + data_pkt.length, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                    printf("SEND PACKET %d\n", data_pkt.seq_no);
                }
            } 
            // Check if the received ACK is a duplicate ACK for the packet just before the base
            // If the ack number is equal to base-1, it is a duplicate ACK. Add 1 to ndups. If (ndups == 3), retransmit the packet with sequence number base only
            else if (ack_pkt.ack_no == base - 1) {
                ndups++;
                //Checking the limit of duplicate Acks
                if (ndups == 3) {
                    //Retransmitting the packet with sequence number as the base
                    data_pkt.type = 1;
                    data_pkt.seq_no = base;
                    data_pkt.length = (base < N - 1) ? len : strlen(buffer) - (N - 1) * len;
                    memcpy(data_pkt.data, buffer + base * len, data_pkt.length);
                    //Sending the data packet to the server
                    sendto(sock_fd, &data_pkt, sizeof(int) * 3 + data_pkt.length, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                    printf("RESEND PACKET %d\n", data_pkt.seq_no);
                }
            }
        //setting the timer after processing
            alarm(TIMEOUT_SECS);
        }
    }

    close(sock_fd);
    printf("Data Transfer Completed Successfully!\n");
    return 0;
}



