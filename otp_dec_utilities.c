/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_dec_utilities.c
 * Description: This file provides the initialization of functions
 * used by otp_dec.c
 * Last Modified: August 16, 2019
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
*****************************************************************/

#define _GNU_SOURCE
#include <dirent.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "otp_dec_utilities.h"

/*****************************************************************
 * name: connectToServer
 * Preconditions:
 * @param port - char pointer to the server's port number
 * @param newSocket - pointer to an uninitialized int
 * Postconditions:
 * If a connection cannot be made, the program will terminate.
 * If connection to server is made, newSocket is created and can
 * be used for future communication with the server.
 * Source: https://beej.us/guide/bgnet/html/multi/clientserver.html#simpleclient
 *****************************************************************/
void connectToServer(char *port, int *newSocket) {
    int status;
    struct addrinfo hints, *servInfo, *p;

    memset(&hints, 0, sizeof hints);
    // IPv4
    hints.ai_family = AF_INET;
    // TCP
    hints.ai_socktype = SOCK_STREAM;

    // Get the address information for the server, which can be used to connect
    // Returns non-zero if fails
    if ((status = getaddrinfo("localhost", port, &hints, &servInfo)) != 0) {
        error("FAILED TO GET ADDRESS INFO", true);
    }

    // Loops through addresses until one that can be connected to is found
    for (p = servInfo; p != NULL; p = p->ai_next) {
        // Tries to create a socket using the info from getaddrinfo
        if ((*newSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        // If socket was created, tries to use it to connect to the server
        if (connect(*newSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close(*newSocket);
            continue;
        }
        // Socket was successfully created and connection was made
        // Stop trying
        break;
    }

    // No connection was made
    if (p == NULL) {
        error("FAILED TO CREATE SOCKET", true);
    }

    freeaddrinfo(servInfo);
}

/*****************************************************************
 * Name: sendFile
 * Preconditions:
 * @param socket - socket with connection to otp_dec_d
 * @param fileName - pointer to char array holding filename
 * Postconditions: Checks if the requested filename exists in the
 * directory. If it does, sends file to client over data connection
 * References:
 * https://stackoverflow.com/questions/30440188/sending-files-from-client-to-server-using-sockets-in-c
 * https://stackoverflow.com/questions/11952898/c-send-and-receive-file
 * https://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
 * https://stackoverflow.com/questions/11952898/c-send-and-receive-file
 *****************************************************************/
int sendFile(int* socket, char *fileName) {
    int fileDescriptor, sentBytes = 0;
    struct stat fileStats;
    FILE *sendingFile;
    char fileSize[256];

    // Open the requested file
    if((sendingFile = fopen(fileName, "r")) == NULL){
        error("failed to open file", false);
        return -1;
    }
    // Get file stats
    if (fstat(fileno(sendingFile), &fileStats) < 0) {
        error("failed to get file stats", false);
        return -1;
    }
    sprintf(fileSize, "%d\0", fileStats.st_size);
    char sendBuffer[fileStats.st_size];

    // Send file size
    if (send(*socket, fileSize, sizeof(fileSize), 0) < 0) {
        error("failed to send file size", false);
        return -1;
    }

    // Make sure otp_dec_d received the file size
    if (receiveMessage(socket) != 0) {
        error("failed to get confirmation for file size", false);
        return -1;
    }

    // Read the file and send over data connection
    do {
        memset(sendBuffer, 0, sizeof(sendBuffer));
        sentBytes += fread(sendBuffer, 1, sizeof(sendBuffer), sendingFile);
        if(send(*socket, sendBuffer, sizeof(sendBuffer), 0) == -1) {
            error("failed to send file to client", false);
            return -1;
        }
    } while (sentBytes < fileStats.st_size);

    fclose(sendingFile);
    return 0;
}

/*****************************************************************
 * Name: receiveMessage
 * Preconditions:
 * @param socket - pointer to the socket that is connected
 * Postconditions:
 * If received "-1" from server, this indicates they have closed
 * the connection and so the otp_dec should close theirs. If an
 * error occurs, the connection will be closed. If message is
 * successfully received, it is printed.
 *****************************************************************/
int receiveMessage(int *socket) {
    char inMessage[MAXBUFFERSIZE + 1];
    int numBytes;
    // Symbolizes connection is being closed by server
    char errorMessage[] = "-1";

    // Get message from otp_dec_d
    if ((numBytes = recv(*socket, inMessage, MAXBUFFERSIZE, 0)) == -1) {
        error("failure receiving from otp_dec_d", false);
        return -1;
    } else {
        inMessage[numBytes] = '\0';
    }

    // If connection is not with otp_dec_d
    if (strcmp(inMessage, CONFIRMATION) != 0) {
        error("wrong connection", false);
        return -1;
    }

    return 0;
}

// Make sure all chars in the file are either A-Z or space
// Returns the number of chars for comparison
int validateChars(char* filename) {
    FILE* filePtr;
    int numChars = 0,
        character;

    // Open the file for reading
    if (!(filePtr = fopen(filename, "rt"))) {
        return -1;
    }

    // Read each character from the file
    while((character = fgetc(filePtr)) != EOF) {
        // If not a space or A-Z
        if (character != 32 && (character < 65 || character > 90)) {
            // If it's a newline, reached end of file
            if (character == 10) {
                break;
            }
            // Invalid character found
            return -2;
        }
        numChars++;
    }

    fclose(filePtr);
    return numChars;
}

// Make sure the key file only contains valid chars
// and is at least as long as the cyphertext
void validateKey(char* key, int numCyphertextChars) {
    int keyChars = validateChars(key);

    if (keyChars == -1) {
        // File could not be opened
        error("unable to open key file", true);
    } else if (keyChars == -2) {
        // Invalid character
        error("key file contains bad characters", true);
    } else if (keyChars < numCyphertextChars) {
        // File is too short
        error("key is too short", true);
    }
}

// Make sure the cyphertext only contains valid chars
// https://stackoverflow.com/a/4179717
int validateCyphertext(char* filename) {
    int numChars = validateChars(filename);

    if (numChars == -1) {
        // File could not be opened
        error("unable to open cyphertext file", true);
    } else if (numChars == -2) {
        // Invalid character
        error("cyphertext file contains bad characters", true);
    }

    return numChars;
}

// Get plaintext from otp_dec_d and print to stdout
// https://stackoverflow.com/questions/11952898/c-send-and-receive-file
int receiveFile(int socket) {
    int fileSize, remainingData = 0;
    char buffer[MAXBUFFERSIZE + 1];
    ssize_t len = 0;

    // Get file size for incoming file
    while (len <= 0) {
        len = recv(socket, buffer, MAXBUFFERSIZE, 0);
    }
    buffer[len] = '\0';

    // Send confirmation for file size
    if(send(socket, CONFIRMATION, strlen(CONFIRMATION), 0) == -1) {
        error("unable to send confirmation", false);
        return -1;
    }

    fileSize = atoi(buffer);

    // Make sure we receive as much data as was promised
    remainingData = fileSize;

    // Read data from the buffer and print to stdout
    while (remainingData > 0 && ((len = recv(socket, buffer, MAXBUFFERSIZE, 0)) > 0)) {
        buffer[len] = '\0';
        fprintf(stdout, buffer);
        remainingData -= len;
    }

    return 0;
}

/*****************************************************************
 * Name: error
 * Preconditions:
 * @param errorMessage - string holding message that should be
 * printed describing the error
 * @param terminal - boolean indicating whether the entire program
 * should be terminated
 * Postconditions: The error message will be printed. If terminal
 * is true, the program will be terminated.
 *****************************************************************/
void error(char *errorMessage, bool terminal) {
    fprintf(stderr, "\nError: %s\n", errorMessage);
    fflush(stdout);

    if (terminal) {
        exit(1);
    }
}