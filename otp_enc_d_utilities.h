/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_enc_d_utilities.h
 * Description: This file provides the declaration of functions
 * used by otp_enc_d_utilities.c
 * Last Modified: August 16, 2019
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
 *****************************************************************/

#ifndef OTP_ENC_D_UTILITIES_H
#define OTP_ENC_D_UTILITIES_H

#define _GNU_SOURCE
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#define BACKLOG 5          // Size of pending connections queue
#define MAXBUFFERSIZE 256   // Max message size that can be received
#define CONFIRMATION "ENC"  // Message sent for confirmation

enum SocketTypes {
    WELCOME_SOCKET,
    CONTROL_SOCKET
};

struct Connection {
    bool connected;
    pid_t pid;
    int socket;
};

char* cypherFilename;
char* plainFilename;
char* keyFilename;
int numConnections;
int numClosed;
pid_t pidsClosed[5];

void startUp(int, char [], struct Connection []);
void validateCommandLineArguments(int, struct Connection []);
char* validatePortNumber(char *, struct Connection []);
void createSocket(char *, struct Connection []);
int findEmptyConnection(struct Connection []);
void acceptConnections(struct Connection []);
bool validateConnection(struct Connection [], int);
void processConnection(struct Connection [], int);
void createChild(struct Connection [], int);
int receiveMessage(int *, char *);
void terminateProgram(char *, struct Connection []);
void closeSockets(struct Connection [], int);
void checkForEndedProcesses(struct Connection []);
int receiveFile(int, char*);
int validateChars(char*);
int validatePlaintext(char*);
int validateKey(char*, int);
int encode();
int sendCypher(int);
void createSignalHandler();

#endif //OTP_ENC_D_UTILITIES_H
