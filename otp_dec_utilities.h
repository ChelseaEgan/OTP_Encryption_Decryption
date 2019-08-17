/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: otp_dec_utilities.h
 * Description: This file provides the declaration of functions
 * used by otp_dec.c
 * Last Modified: August 16, 2019
 * SOURCES:
 * Much of my socket code was borrowed from my CS372 project:
 * https://github.com/level5esper/FTPSystem/blob/master/ftutilities.c
 * Much of my child process code was borrowed from CS344 project 3:
 * https://github.com/level5esper/smallsh
*****************************************************************/

#ifndef PROGRAM4_OTP_DEC_UTILITIES_H
#define PROGRAM4_OTP_DEC_UTILITIES_H

#include <stdbool.h>

#define MAXBUFFERSIZE 500 // characters
#define CONFIRMATION "DEC"  // Message sent for confirmation

void connectToServer(char *, int *);
int receiveMessage(int *);
int validateChars(char*);
int validateCyphertext(char*);
void validateKey(char*, int);
int sendFile(int*, char *);
void error(char *, bool);
int receiveFile(int);

#endif //PROGRAM4_OTP_DEC_UTILITIES_H
