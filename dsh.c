/*
 * dsh.c
 *
 *  Created on: Aug 2, 2013
 *      Author: chiu
 */
#include "dsh.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/types.h>
#include <errno.h>
#include <err.h>
#include <sys/stat.h>
#include <string.h>
#include <linux/limits.h> // for PATH_MAX

char **split(char *str, char *delim)
{

    // printf("Input String: %s\n", str);

    char *tempStr = strdup(str); // Duplicate the string for safe operation
    char *tempDelim = strdup(delim);
    char *ptr = strstr(tempStr, delim); // strstr() finds the first occurrence of the substring needle in the string haystack.
    int numTokens = 1;

    // Counting the number of tokens
    while (ptr != NULL)
    {
        numTokens++;
        ptr = strstr(ptr + 1, delim);
    }

    // printf("Num tokens: %d\n", numTokens);

    char **array = (char **)malloc((numTokens + 1) * sizeof(char *)); // array of points to strings

    char *token = strtok(tempStr, tempDelim); // get first token

    array[0] = (char *)malloc(strlen((token) + 1) * sizeof(char)); // malloc room for first substring

    int counter = 0;

    while (token != NULL)
    {

        array[counter] = (char *)malloc((strlen(token) + 1) * sizeof(char)); // malloc room for additional substrings
        strcpy(array[counter], token);                                       // copy token into array
        // printf("Added: %s to spot %d\n", token, counter);

        token = strtok(NULL, delim);
        counter++;
    }

    // printf("Counter: %d\n", counter);
    array[counter] = NULL; // set final element to NULL

    return array;
}

// questions: how do I get the arguments

void configureMode(char **tokens)
{
    int newSize = arraySize(tokens) - 1; // size of arguments array

    char *firstString = tokens[0];
    char *secondString = tokens[1];
    char firstChar = firstString[0];

    char *lastString = tokens[newSize]; // last will be NULL, 2nd to last is last

    // printf("first string: %s\n", firstString);

    // printf("first char: %c\n", firstChar);

    if (firstChar == '/') // if this triggers, firstString is the path
    {

        if (access(firstString, F_OK | X_OK) == 0)
        {

            // pid_t parent = getpid();
            pid_t pid = fork(); // when you fork, you store the pid of the child.

            if (pid == -1)
            {
                printf("Failed to fork!\n");
            }
            else if (pid > 0) // parent
            {

                if (*lastString != '&')
                {
                    printf("waiting!\n");
                    waitpid(pid, NULL, 0); // waiting for the child to finish
                }
                printf("parents picking back up...\n");
            }
            else // child
            {

                // example: /bin/ls or /usr/bin/ls

                if (newSize > 0) // if there are arguments
                {
                    // printf("New size: %d\n", newSize);
                    char **arguments = malloc(newSize * sizeof(char *)); // 2d array of args to pass to execv

                    int i = 1;
                    while (tokens[i] != NULL) // populating
                    {

                        arguments[i - 1] = strdup(tokens[i]); // strdup allocates memory and copies the string
                        i++;
                    }

                    execv(firstString, arguments);
                    _exit(EXIT_FAILURE); // exec never returns
                }
                else
                {
                    execv(firstString, tokens); // passing it tokens. maybe a bad idea
                    _exit(EXIT_FAILURE);
                }
            }
        }
        else
        {
            printf("Error accessing file!\n");
            // No good! File doesn't exist or is not executable!
            // Alert the user and re-prompt
        }
    }

    else if ((strcmp(firstString, "exit") == 0))
    {
        exit(0);
    }
    else if (strcmp(firstString, "pwd") == 0)
    {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("Current working directory: %s\n", cwd);
        }
        else
        {
            perror("getcwd() error");
        }
    }

    else if (strcmp(firstString, "cd") == 0)
    {

        if (secondString == NULL)
        {

            chdir(getenv("HOME"));
        }
        else if (chdir(secondString) == 0)
        {
            // printf("Changed the current working directory to: %s\n", secondString);
        }
        else
        {
            // printf("uhh getting here?");
            perror("chdir failed");
        }
    }
}

int arraySize(char **array) // took most of this method from geeks for geeks
{
    int count = 0;
    while (array[count] != NULL)
    {
        count++;
    }
    return count;

    return count;
}

void prompt()
{
    while (1)
    {

        char *cmdline = (char *)malloc(256);
        printf("\ndsh> ");
        fgets(cmdline, 256, stdin);
        cmdline[strlen(cmdline) - 1] = 0;
        char **terms = split(cmdline, " ");

        configureMode(terms);

        int i = 0;
        while (terms[i] != NULL)
        {
            // printf("Term %d: %s\n", i, terms[i]);
            i++;
        }
        free(cmdline);
    }
}
