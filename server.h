/*
 * server.h
 * Server functions to simplify connections and initialization.
 *
 * Zach Sheppard (zsheppa)
 * Due: 1-30-2016
 * CPSC 360
 */

#pragma once

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 32
#define CLIENT_SIZE 100
#define RAND_LIMIT 1000000000

// handle interrupt
void exit_statistics();

// return ip address (dotted)
char* get_address(struct sockaddr_in*);

// initialize the socket connection
void init_connection(struct sockaddr_in*, unsigned int);

// generate a random number between 0 and 1,000,000,000
unsigned int rand_goal(); 

// sends a return code to the client (0, 1, or 2)
int send_return(int, struct sockaddr_in*, unsigned int);
