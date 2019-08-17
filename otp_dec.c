/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_dec.c
 * Description: This program takes an encoded file and a key and
 * sends them to otp_dec_d to get decoded.
 * Last Modified: August 16, 2019
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
*****************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>

#include "otp_dec_utilities.h"

int main(int argc, char *argv[]) {
    int newSocket,
        exitCode = 0,
        numChars = 0;
    char username[13];

    // User should have entered four arguments on the command line:
    // program name, cyphertext filename, key filename, and port
    if (argc < 4) {
        printf("Please try again with the following format:\n");
        printf("otp_dec <cyphertext> <key> <port>");
        fflush(stdout);
        return 1;
    }

    // Set the filenames and port to the user's input
    char *cyphertext = argv[1];
    char *key = argv[2];
    char *port = argv[3];

    // Make sure the cyphertext does not contain invalid characters
    numChars = validateCyphertext(cyphertext);
    // Make sure the key does not contain invalid characters and is
    // at least as large as cyphertext
    validateKey(key, numChars);

    // Create a socket and connect to server
    connectToServer(port, &newSocket);

    // Continues until error is encountered or all functions have completed
    while (true) {
        // Get confirmation that it is connected to otp_dec_d
        if ((receiveMessage(&newSocket)) != 0) {
            exitCode = 2;
            break;
        }

        // Confirm with otp_dec_d that everything is good to go
        if (send(newSocket, CONFIRMATION, strlen(CONFIRMATION), 0) == -1) {
            error("failed to send confirmation to otp_dec_d", false);
            exitCode = 1;
            break;
        }

        // Send the cyphertext to otp_dec_d
        if ((sendFile(&newSocket, cyphertext) != 0)) {
            exitCode = 1;
            break;
        }

        // Get confirmation that file was received
        if ((receiveMessage(&newSocket)) != 0) {
            exitCode = 1;
            break;
        }

        // Send the key file to otp_dec_d
        if ((sendFile(&newSocket, key) != 0)) {
            exitCode = 1;
            break;
        }

        // Get confirmation that file was received
        if ((receiveMessage(&newSocket)) != 0) {
            exitCode = 1;
            break;
        }

        // Get the decoded file from otp_dec_d
        if (receiveFile(newSocket) != 0) {
            exitCode = 1;
            break;
        }

        break;
    }

    // Close the connection with the server
    close(newSocket);

    return exitCode;
}