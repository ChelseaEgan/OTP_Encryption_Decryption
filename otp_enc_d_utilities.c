/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_enc_d_utilities.c
 * Description: This file provides the initialization of functions
 * used by otp_enc_d.c
 * Last Modified: August 16, 2019
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
*****************************************************************/
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "otp_enc_d_utilities.h"

/*****************************************************************
 * name: startUp
 * Preconditions:
 * @param argNum - integer indicating number of command line args
 * @param port - port number provided on command line
 * @param sockets - array of sockets used by program
 * Postconditions: Validates number of args provided and the port
 * number then creates a welcoming socket.
 *****************************************************************/
void startUp(int argNum, char *port, struct Connection sockets[]) {
    // Set the port of the server to the one selected if valid
    validateCommandLineArguments(argNum, sockets);
    char* controlPort = validatePortNumber(port, sockets);

    // Create a socket to listen for connection requests
    createSocket(controlPort, sockets);
}

/*****************************************************************
 * name: validateCommandLineArguments
 * Preconditions:
 * @param numArgs - integer indicating number of command line args
 * @param sockets - array of sockets used by program
 * Postconditions: If the numArgs is not 2, terminates the program
 * as it was started incorrectly.
 *****************************************************************/
void validateCommandLineArguments(int numArgs, struct Connection sockets[]) {
    // User should have entered two arguments on the command line:
    // file name and port number
    if (numArgs < 2) {
        terminateProgram("TRY AGAIN WITH otp_enc_d <port>", sockets);
    }
}

/*****************************************************************
 * name: validatePortNumber
 * Preconditions:
 * @param port - char pointer to the command line argument
 * @param sockets - array of sockets used by program
 * Postconditions: Returns the port number if it is valid and
 * within the range of 1024-65535. Otherwise, it terminates the
 * program.
 *****************************************************************/
char* validatePortNumber(char *port, struct Connection sockets[]) {
    if(atoi(port) < 1024 || atoi(port) > 65535) {
        terminateProgram("OUT OF RANGE! USE AN INT BETWEEN 1024-65535.", sockets);
    } else {
        return port;
    }
}

/*****************************************************************
 * name: createSocket
 * Preconditions:
 * @param port - char pointer to the selected port number
 * @param sockets - array of sockets used by program
 * Postconditions: Creates a "welcoming socket" to listen for
 * incoming connection requests. If the setup fails, the program
 * terminates.
 * Source: https://beej.us/guide/bgnet/html/multi/clientserver.html#simpleserver
 * Source for handling zombie children: https://stackoverflow.com/a/18437957
 *****************************************************************/
void createSocket(char *port, struct Connection sockets[]) {
    struct addrinfo hints, *servInfo, *p;
    int addressInfo;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;        // Autofills IP address

    // Get the address information for the host
    if((addressInfo = getaddrinfo(NULL, port, &hints, &servInfo)) != 0) {
        terminateProgram("FAILED TO GET ADDRESS INFO", sockets);
    }

    for(p = servInfo; p != NULL; p = p->ai_next) {
        // Creates the welcoming socket
        if((sockets[WELCOME_SOCKET].socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        // Sets options for the socket
        if(setsockopt(sockets[WELCOME_SOCKET].socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            terminateProgram("FAILED TO SET SOCKET OPTIONS", sockets);
        }

        // Binds to created welcoming socket
        if(bind(sockets[WELCOME_SOCKET].socket, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockets[WELCOME_SOCKET].socket);
            continue;
        }

        break;
    }

    freeaddrinfo(servInfo);

    // Socket creation was unsuccessful - terminates the whole program
    if (p == NULL) {
        terminateProgram("SOCKET CREATION FAILED", sockets);
    }

    // Listening was unsuccessful - terminates the whole program
    if (listen(sockets[WELCOME_SOCKET].socket, BACKLOG) == -1) {
        terminateProgram("FAILED TO LISTEN TO SOCKET", sockets);
    }

    sockets[WELCOME_SOCKET].connected = true;

    // Silently reap forked children instead of turning it into a zombie
    signal(SIGCHLD, SIG_IGN);
}

// Find a Connection struct that does not contain an open socket
int findEmptyConnection(struct Connection sockets[]) {
    for (int i = 1; i < BACKLOG + 1; i++) {
        if (!sockets[i].connected) {
            return i;
        }
    }

    // No open sockets found
    return -5;
}

/*****************************************************************
 * Name: acceptConnections
 * Preconditions:
 * @param sockets - array of sockets used by program
 * Postconditions: Creates a control socket to otp_enc.
 * Terminates the program if fails.
 * Source: https://beej.us/guide/bgnet/html/multi/clientserver.html#simpleserver
 *****************************************************************/
void acceptConnections(struct Connection sockets[]) {
    socklen_t sinSize;
    struct sockaddr_storage theirAddressInfo;
    char theirAddress[INET6_ADDRSTRLEN];
    int socketIndex;

    // If there is an empty slot for the connection (5 possible)
    if ((socketIndex = findEmptyConnection(sockets)) > 0) {
        // Accept incoming connection request and create control socket
        sinSize = sizeof(theirAddressInfo);
        sockets[socketIndex].socket = accept(sockets[WELCOME_SOCKET].socket, (struct sockaddr *)&theirAddressInfo, &sinSize);
        if(sockets[socketIndex].socket != -1) {
            // Validate connection and start child process
            processConnection(sockets, socketIndex);
        }
    }
}

// Used to confirm connection and receipt of files from otp_dec
bool sendConfirmation(int socket) {
    if(send(socket, CONFIRMATION, strlen(CONFIRMATION), 0) == -1) {
        fprintf(stderr, "Error: failed to send confirmation\n");
        fflush(stderr);
        close(socket);
        return false;
    }

    // Successfully sent confirmation
    return true;
}

// Used to get confirmation from otp_enc
bool receiveConfirmation(int socket) {
    char response[MAXBUFFERSIZE + 1];

    // Get message from buffer
    if(receiveMessage(&socket, response) == -1) {
        close(socket);
        return false;
    }

    // If the message was not a confirmation, something failed
    if (strcmp(CONFIRMATION, response) != 0) {
        close(socket);
        return false;
    }

    return true;
}

// Confirm connection with otp_enc
bool validateConnection(struct Connection sockets[], int socketIndex) {
    // Send to connected program that this is the encode daemon
    if (!sendConfirmation(sockets[socketIndex].socket)) {
        return false;
    }

    // Get confirmation that connected to otp_dec
    if(!receiveConfirmation(sockets[socketIndex].socket)) {
        return false;
    }

    // Update socket struct
    sockets[socketIndex].connected = true;

    // Update number of open connections
    numConnections++;
    return true;
}

// Make sure the connection worked then start the child process
void processConnection(struct Connection sockets[], int socketIndex) {
    if (validateConnection(sockets, socketIndex)) {
        createChild(sockets, socketIndex);
    }
}

// Delete all temporary filse
void deleteFiles() {
    remove(cypherFilename);
    remove(plainFilename);
    remove(keyFilename);
}

// Creates child processes, which receive the plaintext and key then returns
// the encoded cyphertext
// https://stackoverflow.com/questions/25261/set-and-oldset-in-sigprocmask
void createChild(struct Connection sockets[], int socketIndex) {
    pid_t spawnpid = -5; // holds ID of child process

    // Create new process
    spawnpid = fork();
    switch (spawnpid) {
        case -1:
            // Fork failed
            fprintf(stderr, "THEY TOOK MY SQUEEZIN' ARM!\n");
            fflush(stderr);
            struct Connection socketArray[] = {sockets[socketIndex]};
            closeSockets(socketArray, 1);
            break;
        case 0:
            // Create filenames using the process ID
            cypherFilename = (char *)malloc(24 * sizeof(char));
            sprintf(cypherFilename, "enc_cyphertext%d\0", getpid());
            plainFilename = (char *)malloc(24 * sizeof(char));
            sprintf(plainFilename, "enc_plaintext%d\0", getpid());
            keyFilename = (char *)malloc(24 * sizeof(char));
            sprintf(keyFilename, "enc_key%d\0", getpid());

            // Get plaintext from otp_enc
            if (receiveFile(sockets[socketIndex].socket, plainFilename) == -1) {
                fprintf(stderr, "Error: failed to receive plaintext file\n");
                fflush(stderr);
                exit(1);
            }

            // Inform otp_enc of successful receipt of file
            if (!sendConfirmation(sockets[socketIndex].socket)) {
                fprintf(stderr, "Error: failed to send confirmation for plaintext file\n");
                fflush(stderr);
                exit(1);
            }

            // Get key from otp_enc
            if (receiveFile(sockets[socketIndex].socket, keyFilename) == -1) {
                fprintf(stderr, "Error: failed to receive key file\n");
                fflush(stderr);
                exit(1);
            }

            // Inform otp_enc of successful receipt of file
            if (!sendConfirmation(sockets[socketIndex].socket)) {
                fprintf(stderr, "Error: failed to send confirmation for key file\n");
                fflush(stderr);
                exit(1);
            }

            // Check that plaintext contains all valid cahracters
            int numChars = validatePlaintext(plainFilename);

            // Plaintext is invalid
            if (numChars == -1) {
                fprintf(stderr, "Error: plaintext is invalid\n");
                fflush(stderr);
                exit(1);
            }

            // Check that key is at least as large as plaintext and is valid
            if (validateKey(keyFilename, numChars) == -1) {
                fprintf(stderr, "Error: key file is invalid\n");
                fflush(stderr);
                exit(1);
            }

            // Encode the plaintext
            if (encode() != 0) {
                fprintf(stderr, "Error: failed to encode plaintext file\n");
                fflush(stderr);
                exit(1);
            }

            // Send the encoded file to otp_enc
            if (sendCypher(sockets[socketIndex].socket) != 0) {
                fprintf(stderr, "Error: failed to send cyphertext file\n");
                fflush(stderr);
                exit(1);
            }

            // Delete all temporary files
            deleteFiles();

            // Free allocated memory and end the process
            free(cypherFilename);
            free(plainFilename);
            free(keyFilename);
            exit(0);
            break;
        default:
            // Catch ending child processes
            createSignalHandler();
            // Update the socket struct
            sockets[socketIndex].pid = spawnpid;
            break;
    }
}

// Reset the socket structs for any closed sockets
// http://www.microhowto.info/howto/reap_zombie_processes_using_a_sigchld_handler.html
void checkForEndedProcesses(struct Connection sockets[]) {
    // Loop through the five possible closed connections
    for (int i = 0; i < 5; i++) {
        // If this element contains a valid pid
        if (pidsClosed[i] > 0) {
            // Loop through the five possible sockets
            for (int j = 1; j < BACKLOG + 1; j++) {
                // If this is the socket that was closed, reset
                if (sockets[j].pid == pidsClosed[i]) {
                    sockets[j].pid = -5;
                    close(sockets[j].socket);
                    sockets[j].connected = false;
                    sockets[j].socket = 0;
                    break;
                }
            }

            // Indicates that this process closure was handled
            numClosed--;
            pidsClosed[i] = -5;
            numConnections--;
        }
    }
}

/*****************************************************************
 * Name: receiveMessage
 * Preconditions:
 * @param socket - pointer to the socket that is connected
 * @param buffer - pointer to the char array that will hold the
 * incoming message
 * Postconditions: Reads from buffer
 *****************************************************************/
int receiveMessage(int *socket, char *buffer) {
    int numBytes;

    // Put bytes into buffer
    if ((numBytes = recv(*socket, buffer, MAXBUFFERSIZE, 0)) == -1) {
        return -1;
    }

    // Append with a null terminator
    buffer[numBytes] = '\0';
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
// and is at least as long as the plaintext
int validateKey(char* key, int numPlaintextChars) {
    int keyChars = validateChars(key);

    if (keyChars == -1) {
        // File could not be opened
        return -1;
    } else if (keyChars == -2) {
        // Invalid character
        return -2;
    } else if (keyChars < numPlaintextChars) {
        // File is too short
        return -1;
    }

    return 0;
}

// Make sure the plaintext only contains valid chars
// https://stackoverflow.com/a/4179717
int validatePlaintext(char* filename) {
    int numChars = validateChars(filename);

    if (numChars == -1) {
        // File could not be opened
        return -1;
    } else if (numChars == -2) {
        // Invalid character
        return -2;
    }

    return numChars;
}

// Get data from otp_enc and save it in a file
// https://stackoverflow.com/questions/11952898/c-send-and-receive-file
int receiveFile(int socket, char* filename) {
    int fileSize, remainingData = 0;
    char buffer[MAXBUFFERSIZE + 1];
    FILE* receivedFile;
    ssize_t len = 0;

    // Get file size for incoming file
    while (len <= 0) {
        len = recv(socket, buffer, MAXBUFFERSIZE, 0);
    }
    buffer[len] = '\0';

    // Send confirmation for file size
    if (!sendConfirmation(socket)) {
        return -1;
    }

    fileSize = atoi(buffer);

    // Open a file to write the data
    if((receivedFile = fopen(filename, "w")) == NULL){
        return -1;
    }

    // Make sure we receive as much data as was promised
    remainingData = fileSize;

    // Read data from the buffer and write it to the file
    while (remainingData > 0 && ((len = recv(socket, buffer, MAXBUFFERSIZE, 0)) > 0)) {
        buffer[len] = '\0';
        fwrite(buffer, sizeof(char), len, receivedFile);
        remainingData -= len;
    }

    fclose(receivedFile);
    return 0;
}

// Encodes the plaintext and writes to the cyphertext file
int encode() {
    int plaintextChar, keyChar, codeChar;
    FILE* cypherFile;
    FILE* plaintextFile;
    FILE* keyFile;

    // Open the cyphertext file to write encoded data
    if ((cypherFile = fopen(cypherFilename, "w")) == NULL) {
        return -1;
    }

    // Open plaintext
    if ((plaintextFile = fopen(plainFilename, "r")) == NULL) {
        return -1;
    }

    // Open keyfile
    if ((keyFile = fopen(keyFilename, "r")) == NULL) {
        return -1;
    }

    // Get each char from the cyphertext until newline
    plaintextChar = fgetc(plaintextFile);
    while (plaintextChar != 10) {
        // Get the corresponding key char
        keyChar = fgetc(keyFile);

        // If the char is a space, change ASCII to 26
        if (plaintextChar == 32) {
            plaintextChar = 26;
        } else {
            // Convert the ASCII code to 0-25
            plaintextChar -= 65;
        }

        // If the char is a space, change ASCII to 26
        if (keyChar == 32) {
            keyChar = 26;
        } else {
            // Convert the ASCII code to 0-25
            keyChar -=65;
        }

        // Encoded char is found by adding the key ASCII to the plaintext char
        codeChar = plaintextChar + keyChar;

        // If the char is greater than 26, subtract 27 to get the correct encoded ASCII
        if (codeChar > 26) {
            codeChar -= 27;
        }

        // If 26, then change to ASCII space code 32
        if (codeChar == 26) {
            codeChar = 32;
        } else {
            // Convert to actual ASCII code
            codeChar += 65;
        }

        fputc(codeChar, cypherFile);
        plaintextChar = fgetc(plaintextFile);
    }

    // Finish with newline character
    fputc('\n', cypherFile);

    // Close all opened files
    fclose(cypherFile);
    fclose(plaintextFile);
    fclose(keyFile);

    return 0;
}

// Send the encoded file to otp_enc
int sendCypher(int socket) {
    int fileDescriptor, sentBytes = 0;
    struct stat fileStats;
    FILE *sendingFile;
    char fileSize[256];

    // Open the requested file
    if((sendingFile = fopen(cypherFilename, "r")) == NULL){
        return -1;
    }

    // Get file stats
    if (fstat(fileno(sendingFile), &fileStats) < 0) {
        return -1;
    }

    sprintf(fileSize, "%d\0", fileStats.st_size);
    char sendBuffer[fileStats.st_size];

    // Send file size
    if (send(socket, fileSize, sizeof(fileSize), 0) < 0) {
        return -1;
    }

    // Make sure otp_enc received file size
    if (!receiveConfirmation(socket)) {
        return -1;
    }

    // Read the file and send over data connection
    do {
        memset(sendBuffer, 0, sizeof(sendBuffer));
        sentBytes += fread(sendBuffer, 1, sizeof(sendBuffer), sendingFile);
        if(send(socket, sendBuffer, sizeof(sendBuffer), 0) == -1) {
            return -1;
        }
    } while (sentBytes < fileStats.st_size);

    fclose(sendingFile);
    return 0;
}
/*****************************************************************
 * Name: terminateProgram
 * Preconditions:
 * @param errorMessage - string holding message that should be
 * printed describing the error
 * @param sockets - array of sockets used by program
 * Postconditions: The error message will be printed and then
 * all sockets closed.
 *****************************************************************/
void terminateProgram(char *errorMessage, struct Connection sockets[]) {
    fprintf(stderr, "Error: %s\n", errorMessage);
    fflush(stderr);

    closeSockets(sockets, numConnections + 1);

    exit(1);
}

/*****************************************************************
 * Name: closeSockets
 * Preconditions:
 * @param sockets - array of sockets to be closed
 * @param numSockets - number of sockets to be closed
 * Postconditions: Closes provided sockets
 *****************************************************************/
void closeSockets(struct Connection sockets[], int numSockets) {
    int i = 0;

    for(i; i < numSockets; i++) {
        close(sockets[i].socket);
    }
}

// Handles SIGCHLD signals
void sigChldHandler(int signo) {
    int childExitMethod;
    pid_t pid = -5;

    // Loop until no more ending processes
    while ((pid = waitpid((pid_t)(-1), &childExitMethod, 0)) > 0) {
        pidsClosed[numClosed] = pid;

        // Indicate that there are ended processes that need to be closed
        numClosed++;
    }
}

// Create the signal handlers for SIGCHLD (handled by sigChldHandler)
// http://web.engr.oregonstate.edu/~brewsteb/CS344Slides/3.3%20Signals.pdf
void createSignalHandler() {
    struct sigaction SIGCHLD_action = {0};
    SIGCHLD_action.sa_handler = &sigChldHandler;
    sigfillset(&SIGCHLD_action.sa_mask);
    SIGCHLD_action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);
}