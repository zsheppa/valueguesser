/*
 * client.c
 * Connects to the corresponding server and attempts to guess a number.
 *
 * Zach Sheppard (zsheppa)
 * Due: 1-30-2016
 * CPSC 360
 */

#include "client.h"

// guess count
static unsigned int attempts = 0;

// file descriptor for socket initialization
static int descriptor = -1;
    
// guess number that will eventually become the solution
static unsigned int guess = 0;
    
// timer for how long it took to guess
static struct timeval guess_time;

// set timeout value for each message
struct timeval timeout;

/*
    * valueGuesser -s <serverIP> -p1 <serverPort1> -p2 <serverPort2>
    * serverIP: The IP address (in dotted name format) of the host that is running the server side.
    * serverPort1: The 16 bit port number the server is using. You should generally run the server with a port number in the 5K to 10K range.
    * serverPort2: The secondary port number the server is using.
*/
int main(int argc, char **argv) {
    
    // low and high variables for the binary search
    struct Bounds bounds;
    
    // return code retrieved
    int return_code = 0;
    
    // server sockaddr
    struct sockaddr_in server_addr;
    
    // server ip address to connect to
    char* server_ip = 0;
    
    // server port to communicate with
    unsigned int server_port = 0;
    
    // backup server port in the case that the provided port fails
    unsigned int server_port_backup = 0;

    // seed the random number generator
    srand(time(NULL));
    
    // check argument values
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -s <server ip> -p1 <server port 1> -p2 <server port 2 (backup)>", argv[0]);
        return 1;
    }
    // assign argument values
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-s", 2) == 0) {
            if (argc < (i+2)) {
                fprintf(stderr, "You must give an argument\n");
                return 1;
            }
            server_ip = argv[++i];
        } else if (strncmp(argv[i], "-p1", 3) == 0) {
            if (argc < (i+2)) {
                fprintf(stderr, "You must give an argument\n");
                return 1;
            }
            server_port = atoi(argv[++i]);
        } else if (strncmp(argv[i], "-p2", 3) == 0) {
            if (argc < (i+2)) {
                fprintf(stderr, "You must give an argument\n");
                return 1;
            }
            server_port_backup = atoi(argv[++i]);
        } else {
            fprintf(stderr, "The argument '%s' is not supported\n", argv[i]);
            return 1;
        }
    }

    // register the SIGINT handler
    signal(SIGINT, exit_statistics);
    
    // check to see if the port is open
    if (test_port(server_ip, server_port) == 0) {
    
        if (server_port_backup == 0) {
            fprintf(stderr, "The primary port (%d) on %s is not available\n", server_port, server_ip);
            return 1;
        }
        
        // if port is not open, test the backup
        if (test_port(server_ip, server_port_backup) == 0) {
            fprintf(stderr, "The primary port (%d) and backup port (%d) on %s are not available\n",
                server_port, server_port_backup, server_ip
            );
            return 1;
        } else {
            // if it passes, set the server port to the backup
            server_port = server_port_backup;
            printf("Could not connect to the primary port; connecting to backup\n");
        }
    }
    
    // initialize the socket
    descriptor = init_connection(&server_addr, server_ip, server_port);
    
    // make an initial guess
    guess = initial_guess();
    
    // start the timer
    gettimeofday(&guess_time, 0);
    
    do {
        attempts++;
        
        // send the guess to the server
        send_guess(descriptor, &server_addr, guess);
        
        // wait for the return code from the server
        return_code = recv_code(descriptor, &server_addr);
        
        // generate another guess (up to 10e^9)
        guess = next_guess(return_code, guess, &bounds);
        
    } while (return_code != 0);
    
    exit_statistics();
    
    return 0;
}

// prints out ending statistics
void exit_statistics() {
    // capture time
    struct timeval end_time;
    gettimeofday(&end_time, 0);
    
    // compute
    long int total_sec = end_time.tv_sec - guess_time.tv_sec;
    long int total_mic = end_time.tv_usec - guess_time.tv_usec;
    
    // print statistics
    printf("Attempts\tTime\t\tValue\n");
    printf("%d\t\t%ld.%06ld\t%d\n", attempts, total_sec, total_mic, guess);
    
    // close the open socket
    close(descriptor);
    
    exit(0);
}

// opens a socket and returns the descriptor
int init_connection(struct sockaddr_in* server_addr, char* server_ip, unsigned int server_port) {
    // create the socket
    int descriptor = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (descriptor < 0) {
        fprintf(stderr, "There was a problem creating a socket to %s:%d\n", server_ip, server_port);
        exit(1);
    }

    // if no error, assign values to the addr struct
    memset(server_addr, 0, sizeof(*server_addr));
    (*server_addr).sin_family = AF_INET;
    (*server_addr).sin_addr.s_addr = inet_addr(server_ip);
    (*server_addr).sin_port = htons(server_port);
    
    // set timeout option
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;
    setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    
    return descriptor;
}

// makes an initial random guess
unsigned int initial_guess() {
    unsigned int r = 0;
    
    do {
        r = rand();
    } while (r < RAND_LIMIT);
    
    return r;
}

// generates a guess
unsigned int next_guess(int code, unsigned int previous, struct Bounds* bounds) {
    if (code == 1) {
        bounds->high = previous;
    } else if (code == 2) {
        bounds->low = previous + 1;
    }
    
    return ((bounds->low + bounds->high) / 2);
}

// receives a return code from the server
int recv_code(int descriptor, struct sockaddr_in* server_addr) {
    // declare the buffers to write to
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    
    // receive the return code from the server
    socklen_t server_addr_length = sizeof(*server_addr); // this is weird, but required
    int bytes_recv = recvfrom(descriptor, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, &server_addr_length);
    if (bytes_recv < 0) {
        // if we timed out, print out an error message
        if (errno == 11) {
            fprintf(stderr, "Waited %d second(s) for response; trying again\n", TIMEOUT_SECONDS);
        } else {
            fprintf(stderr, "There was a problem receiving the return code from the server\n");
        }
        return bytes_recv;
    }
    
    // once a message is received it should be:
    //     CODE x
    // if not, invalid and should be skipped
    int code = 0;
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
    
    // if result is not CODE, continue
    if (strncmp(splitted, "CODE", 4) != 0) {
        fprintf(stderr, "The message received from the server was not valid\n");
        return -1;
    } else {
        // if result is guess, store the number
        int splitpoint = strlen(splitted) + 1;
        memset(splitted, '\0', BUFFER_SIZE);
        int z = 0;
        for (i = splitpoint; i < strlen(buffer); i++) {
            splitted[z++] = buffer[i];
        }
        code = atoi(splitted);
    }
    
    return code;
}

// sends a guess to the server
void send_guess(int descriptor, struct sockaddr_in* server_addr, unsigned int guess) {
    // declare the command
    char command[7] = "GUESS ";
    
    // declare the buffers to write to
    char buffer[BUFFER_SIZE];
    char guess_buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    memset(guess_buffer, '\0', BUFFER_SIZE);
    
    // copy the command to the buffer
    strncpy(buffer, command, BUFFER_SIZE);
    
    // copy the guess into the guess buffer
    snprintf(guess_buffer, BUFFER_SIZE, "%u", guess);
    
    // concatenate both the command and guess
    strcat(buffer, guess_buffer);
    
    sendto(descriptor, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
}

// tests a connection to the sockaddr passed
//      0: connection failed
//      1: connection succeeded
unsigned int test_port(char* server_ip, unsigned int server_port) {
    // declare a test socket
    struct sockaddr_in test_addr;
    int testd = init_connection(&test_addr, server_ip, server_port);
    
    // determine the result when we attempt to connect
    int result = connect(testd, (struct sockaddr *)&test_addr, sizeof(test_addr));
    
    close(testd);
    
    return (result == 0);
}
