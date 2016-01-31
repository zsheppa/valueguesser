/*
 * server.c
 * Runs the server program that generates a positive integer value and allows clients
 * to connect to it and guess the value.
 *
 * Zach Sheppard (zsheppa)
 * Due: 1-30-2016
 * CPSC 360
 */

#include "server.h"

// count of clients that have connected
static unsigned int client_count = 0;

// list of clients that have connected
static char* client_list[CLIENT_SIZE];

// socket descriptor for the server
int descriptor = -1;

// count of correct guesses
static unsigned int guess_count = 0;

// count of messages received
static unsigned int message_count = 0;

int main(int argc, char** argv) {
    
    // message buffer
    char buffer[BUFFER_SIZE];
    
    // client's address sock address struct
    struct sockaddr_in client_addr;
    
    // length of client_addr to prevent an incompatible type error
    unsigned int client_addr_length = sizeof(client_addr);
    
    // goal number (sometimes generated, sometimes defined)
    int goal = -1;
    
    // size of the message received
    int message_size = 0;
    
    // port number to listen on
    unsigned int port = 0;
    
    // server's address sock address struct
    struct sockaddr_in server_addr;
    
    // seed the random number
    srand(time(NULL));
    
    // server -p <serverPort> -v <initialValue>
    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: %s -p <serverPort> -v <initialValue> (optional)\n", argv[0]);
        return 1;
    } else {
        // get arguments
        for (int i = 1; i < argc; i++) {
            if (strncmp(argv[i], "-p", 2) == 0) {
                if (argc < (i+2)) {
                    fprintf(stderr, "You must give an argument\n");
                    return 1;
                }
                port = atoi(argv[++i]);
            } else if (strncmp(argv[i], "-v", 2) == 0) {
                if (argc < (i+2)) {
                    fprintf(stderr, "You must give an argument\n");
                    return 1;
                }
                goal = atoi(argv[++i]);
                if (goal < 0) {
                    fprintf(stderr, "The initial value must be between 0 and 1,000,000,000\n");
                    return 1;
                 }
            } else {
                fprintf(stderr, "The argument '%s' is not supported\n", argv[i]);
                return 1;
            }
        }
    }
    
    // check if port has been provided
    if (port == 0) {
        fprintf(stderr, "A port must be specified with -p\n");
        return 1;
    // also check to see if an initial goal has been provided
    } else if (goal == -1) {
        // if not, generate one
        goal = rand_goal();
    }
    
    printf("Initial goal: %d\n", goal);
    
    init_connection(&server_addr, port);
    
    printf("Now listening on %s:%d\n\n", get_address(&server_addr), port);

    // initialize the client list
    memset(client_list, '\0', sizeof(client_list));
    
    // handle interrupt
    signal(SIGINT, exit_statistics);
    
    // server loop
    while (1) {
        // block here until a message is received
        message_size = recvfrom(descriptor, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_length);
        if (message_size < 0) {
            fprintf(stderr, "There was a problem receiving data from the socket");
            return 1;
        }
        
        // log this (maybe new?) client
        int ci = 0;
        for (ci = 0; ci < client_count; ci++) {
            char* address = get_address(&client_addr);
            if (strcmp(address, client_list[ci]) == 0) {
                ci = -1;
                break;
            }
        }
         
        // if the client was not found, log it
        if (ci != -1) {
            client_list[client_count++] = get_address(&client_addr);
        }
        
        // increment the total message counter that will be displayed
        // after the server exits
        message_count++;
        
        // once a message is received it should be:
        //     GUESS x
        // if not, invalid and should be skipped
        int guess = 0;
        char splitted[BUFFER_SIZE];
        int i = 0;
        for (i = 0; i < strlen(buffer); i++) {
            if (buffer[i] == ' ') {
                splitted[i] = '\0';
                break;
            } else {
                splitted[i] = buffer[i];
            }
        }
        
        // if result is not GUESS, continue
        if (strncmp(splitted, "GUESS", 5) != 0) {
            continue;
        } else {
            // if result is guess, store the number
            int splitpoint = strlen(splitted);
            memset(splitted, '\0', BUFFER_SIZE);
            int z = 0;
            for (i = splitpoint; i < strlen(buffer); i++) {
                splitted[z++] = buffer[i];
            }
            guess = atoi(splitted);
        }
        
        // check guess against goal
        if (goal == guess) {
            send_return(descriptor, &client_addr, 0);
            
            // guess was correct to increment the counter and create a new goal
            guess_count++;
            goal = rand_goal();
            printf("Client (%s) guessed correctly; created new guess (%d)\n", get_address(&client_addr), goal);
        
        // too high
        } else if (guess > goal) {
            send_return(descriptor, &client_addr, 1);
        
        // too low
        } else if (guess < goal) {
            send_return(descriptor, &client_addr, 2);
        }
    }
    
    exit_statistics();
    
    return 0;
}

// handle an interrupt by printing out the statistics of the connection
void exit_statistics() {
    // print out statistics
    printf("\n#Messages\t#Clients\tIPs\n");
    printf("%d\t\t%d\t\t", message_count, guess_count);
    
    // loop for client IP addresses
    for (int i = 0; i < client_count; i++) {
        if (i == 0) {
            printf("%s", client_list[i]);
        } else {
            printf(",%s", client_list[i]);
        }
     }
     printf("\n");
     
     // close the socket
     close(descriptor);
     
     exit(0);
}

// return ip address (dotted)
char* get_address(struct sockaddr_in* addr) {
    return inet_ntoa(addr->sin_addr);
}

void init_connection(struct sockaddr_in* server_addr, unsigned int port) {
    // create socket connection with SOCK_DGRAM to serve as a datagram (udp) socket
    descriptor = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (descriptor < 0) {
        fprintf(stderr, "There was a problem opening a socket\n");
        exit(1);
    }
    
    // allocate space for the three arguments that go to sockaddr_in
    memset(server_addr, 0, sizeof(*server_addr));
    (*server_addr).sin_family = AF_INET;
    (*server_addr).sin_addr.s_addr = htonl(INADDR_ANY);
    (*server_addr).sin_port = htons(port);
    
    // bind to the port
    if (bind(descriptor, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        fprintf(stderr, "There was a problem binding the server to a port\n");
        exit(1);
    }
}

// generate a random number between 0 and 1,000,000,000
unsigned int rand_goal() {
    unsigned int r = 0;
    do {
        r = rand();
    } while (r > RAND_LIMIT);
    return r;
}

// sends a return code to the client (0, 1, or 2)
//      0: guess is correct
//      1: guess is too high
//      2: guess is too low
int send_return(int descriptor, struct sockaddr_in* client_addr, unsigned int code) {
    // declare the command
    char command[6] = "CODE ";
    
    // declare the buffers to write to
    char buffer[BUFFER_SIZE];
    char code_buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    memset(code_buffer, '\0', BUFFER_SIZE);
    
    // copy the command to the buffer
    strncpy(buffer, command, BUFFER_SIZE);
    
    // copy the return code into the code buffer
    snprintf(code_buffer, BUFFER_SIZE, "%u", code);
    
    // concatenate both the command and code
    strcat(buffer, code_buffer);
    
    //printf("-> %s\n", buffer);
    
    return sendto(descriptor, buffer, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, sizeof(*client_addr));
}
