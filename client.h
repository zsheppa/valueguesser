/*
 * client.c
 * Client headers and initialization functions.
 *
 * Zach Sheppard (zsheppa)
 * Due: 1-30-2016
 * CPSC 360
 */
 
#pragma once

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 32
#define RAND_LIMIT 1000000000
#define TIMEOUT_SECONDS 1

// bound definition for client
struct Bounds {
    unsigned int low;
    unsigned int high;
};

// print out final statistics
void exit_statistics();

// initialize a socket connection
int init_connection(struct sockaddr_in*, char*, unsigned int);

// generate a random number between 0 and 1,000,000,000
unsigned int initial_guess();

// generate a random number between 0 and a limit depending on the return code
unsigned int next_guess(int, unsigned int, struct Bounds*);

// receive CODE x
int recv_code(int, struct sockaddr_in*);

// send GUESS x
void send_guess(int, struct sockaddr_in*, unsigned int);

// tests a connection to the sockaddr passed
//      0: connection failed
//      1: connection succeeded
unsigned int test_port(char*, unsigned int);

