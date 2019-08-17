/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_enc.c
 * Description: This program takes a plaintext file and a key and
 * sends them to otp_enc_d to get encoded.
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

#include "otp_enc_utilities.h"

int main(int argc, char *argv[]) {
    int newSocket,
        exitCode = 0,
        numChars = 0;
    char username[13];

    // User should have entered four arguments on the command line:
    // program name, cyphertext filename, key filename, and port number
    if (argc < 4) {
        printf("Please try again with the following format:\n");
        printf("otp_enc <plaintext> <key> <port>");
        fflush(stdout);
        return 1;
    }

    // Set the filenames and port to the user's input
    char *plaintext = argv[1];
    char *key = argv[2];
    char *port = argv[3];

    // Make sure the plaintext does not contain invalid characters
    numChars = validatePlaintext(plaintext);
    // Make sure the key does not contain invalid characters and is
    // at least as large as plaintext
    validateKey(key, numChars);

    // Create a socket and connect to server
    connectToServer(port, &newSocket);

    // Continues until error is encountered or all functions have completed
    while (true) {
        // Get confirmation that it is connected to otp_enc_d
        if ((receiveMessage(&newSocket)) != 0) {
            exitCode = 2;
            break;
        }

        // Confirm with otp_enc_d that everything is good to go
        if (send(newSocket, CONFIRMATION, strlen(CONFIRMATION), 0) == -1) {
            error("failed to send confirmation to otp_enc_d", false);
            exitCode = 1;
            break;
        }

        // Send the plaintext to otp_enc_d
        if ((sendFile(&newSocket, plaintext) != 0)) {
            exitCode = 1;
            break;
        }

        // Get confirmation that file was received
        if ((receiveMessage(&newSocket)) != 0) {
            exitCode = 1;
            break;
        }

        // Send the key file to otp_enc_d
        if ((sendFile(&newSocket, key) != 0)) {
            exitCode = 1;
            break;
        }

        // Get confirmation that file was received
        if ((receiveMessage(&newSocket)) != 0) {
            exitCode = 1;
            break;
        }

        // Get the encoded file from otp_enc_d
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