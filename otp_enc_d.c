/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_enc_d.c
 * Description: This program takes a plaintext file and a key,
 * encodes it and returns the encoded file
 * Last Modified: August 16, 2019
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
*****************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "otp_enc_d_utilities.h"

int main(int argc, char *argv[]) {
    // Holds information about the socket that is listening for connections and
    // the 5 possible connections
    struct Connection welcomeSocket, controlSocket1, controlSocket2, controlSocket3, controlSocket4, controlSocket5;
    struct Connection sockets[6] = {welcomeSocket, controlSocket1, controlSocket2, controlSocket3, controlSocket4, controlSocket5};
    char *controlPort = argv[1];

    // Tracks number of sockets currently connected to otp_dec
    numConnections = 0;
    // Tracks number of ended processes caught by SIGCHLD handler
    numClosed = 0;

    // Create welcoming socket
    startUp(argc, controlPort, sockets);

    // Run until program is terminated
    while(1) {
        // If anything has been caught by the SIGCHLD handler
        if (numClosed > 0) {
            checkForEndedProcesses(sockets);
        }

        // If there is room for another connection
        if (numConnections < BACKLOG) {
            // Listen for connection requests
            acceptConnections(sockets);
        }
    }

    return 0;
}