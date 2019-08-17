/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_enc_utilities.h
 * Description: This file provides the declaration of functions
 * used by otp_enc.c
 * Last Modified: August 16, 2019
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
*****************************************************************/

#ifndef CHAT_UTILITIES
#define CHAT_UTILITIES

#include <stdbool.h>

#define MAXBUFFERSIZE 500 // characters
#define CONFIRMATION "ENC"  // Message sent for confirmation

void connectToServer(char *, int *);
int receiveMessage(int *);
int validateChars(char*);
int validatePlaintext(char*);
void validateKey(char*, int);
int sendFile(int*, char *);
void error(char *, bool);
int receiveFile(int);

#endif