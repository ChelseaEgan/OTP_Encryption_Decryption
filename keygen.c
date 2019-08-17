/*****************************************************************
 * Name: Chelsea Egan
 * Course: CS 344-400
 * Program: keygen.c
 * Description: This program generates a file filled with a user-
 * defined number of chars (A-Z or space)
 * Last Modified: August 16, 2019
*****************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Verifies that the program received two args and that the second is a positive int
int validateArgs(int numArgs, char* args[]) {
    // Args should be the program name and number of chars in the key
    if (numArgs != 2) {
        fprintf(stderr, "Incorrect number of arguments. The correct format is: keygen <integer>\n");
        return -1;
    }

    // Verify that the second arg is a positive integer
    // https://stackoverflow.com/a/29248688
    int length = strlen(args[1]);
    if (args[1][0] == '-') {
        fprintf(stderr, "Incorrect format of arguments. The correct format is: keygen <positive integer>\n");
        return -1;
    }

    // Verify that each char in the second arg is a digit
    int i;
    for (i = 0; i < length; i++) {
        if (!isdigit(args[1][i])) {
            fprintf(stderr, "Incorrect format of arguments. The correct format is: keygen <integer>\n");
            return -1;
        }
    }

    return 0;
}

// Fills buffer with randomly generated chars of A-Z or space
void generateKey(int numChars, char* keyString) {
    int asciiUppercaseStart = 65,   // Code for 'A'
        asciiUppercaseEnd = 90 + 1, // Code for 'Z' plus one to represent the space
        asciiSpace = 32;            // Code for space

    // Will hold each char, terminates with null terminator for strcat
    char cToStr[2];
    cToStr[1]='\0';

    for (int i = 0; i < numChars; i++) {
        // Generate a code between 65 and 91
        int asciiCode = (rand() % (asciiUppercaseEnd - asciiUppercaseStart + 1)) + asciiUppercaseStart;

        // If the code was 91, change to the code for a space
        if (asciiCode == asciiUppercaseEnd) {
            asciiCode = asciiSpace;
        }

        // Convert the code to a char
        char asciiChar = asciiCode;
        cToStr[0] = asciiChar;

        // Add the char to the end of the keystring
        strcat(keyString, cToStr);
    }

    // Terminate with newline
    strcat(keyString, "\n");
}

int main(int argc, char* argv[]) {
    // Set seed for the key randomization
    srand(time(0));

    // Make sure it's the right number and format of arguments
    if (validateArgs(argc, argv) != 0) {
        exit(1);
    }

    int numKeys = atoi(argv[1]);

    // Create the key
    char* keyString = (char *)malloc((numKeys + 1) * sizeof(char));
    generateKey(numKeys, keyString);

    // Print to stdout
    printf(keyString);
    fflush(stdout);

    free(keyString);
    return 0;
}