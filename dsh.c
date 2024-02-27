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
#include <ctype.h>

/**
 * Splits a string into an array of tokens based on a specified delimiter.
 * The function duplicates the input string for safe tokenization, counts the number of tokens,
 * allocates an array of pointers for the tokens, and then tokenizes the duplicated string,
 * duplicating each token into the array. The original duplicated string is freed before returning the array.
 *
 * @param str The input string to be tokenized.
 * @param delim The delimiter used to split the input string.
 * @return A dynamically allocated array of dynamically allocated token strings, terminated by a NULL pointer.
 */
char **split(char *str, char *delim)
{

    char *tempStr = strdup(str); // Duplicate the string for safe operation

    char *ptr = strstr(tempStr, delim); // strstr() finds the first occurrence of delim

    // Counting the number of tokens
    int numTokens = 1;
    while (ptr != NULL)
    {
        numTokens++;
        ptr = strstr(ptr + 1, delim);
    }

    char **array = (char **)malloc((numTokens + 1) * sizeof(char *)); // Allocate memory for the array of pointers to tokens

    char *token = strtok(tempStr, delim); // Get the first token
    int counter = 0;
    while (token != NULL)
    {
        array[counter] = strdup(token); // Duplicate token
        token = strtok(NULL, delim);    // Get the next token
        counter++;
    }
    array[counter] = NULL; // Set the last element to NULL

    free(tempStr); // Free the duplicated string

    return array;
}

/**
 * Determines the operation mode based on the first token of the input and invokes the corresponding function.
 * It supports direct execution (mode1) if the first token starts with '/', built-in commands like 'exit', 'pwd', and 'cd',
 * or a PATH search for the executable (mode2) for other cases.
 *
 * @param tokens A NULL-terminated array of token strings representing the parsed user command.
 */
void configureMode(char **tokens)
{
    int newSize = arraySize(tokens) - 1; // size of arguments array

    char *firstString = tokens[0];
    char *secondString = tokens[1];
    char firstChar = firstString[0];

    // char *lastString = tokens[newSize]; // last will be NULL, 2nd to last is last

    // printf("first string: %s\n", firstString);

    // printf("first char: %c\n", firstChar);

    if (firstChar == '/') // if this triggers, firstString is the path
    {
        mode1(firstString, tokens, newSize);
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

    else // Mode 2: Full-path construction when the command does not start with '/'
    {
        mode2(firstString, tokens, newSize);
    }
}

/**
 * Executes a command directly if it's specified with an absolute path.
 * It checks for background execution, validates the command, forks a child process to execute the command,
 * and waits for the command to finish if it's not a background process.
 *
 * @param firstString The first token of the command, representing the command to be executed.
 * @param tokens A NULL-terminated array of token strings representing the parsed user command.
 * @param newSize The number of tokens in the 'tokens' array excluding the background indicator '&' if present.
 */
void mode1(char *firstString, char **tokens, int newSize)
{
    printf("Mode 1 triggered with command: %s\n", firstString);

    // Check if the last argument is '&' for background execution
    int background = 0;
    if (newSize > 0 && strcmp(tokens[newSize], "&") == 0)
    {
        background = 1;         // Mark as background execution
        tokens[newSize] = NULL; // Remove '&' from arguments for execv
        newSize--;              // Adjust size to exclude '&'
    }

    // Validate the command
    if (access(firstString, F_OK | X_OK) == 0)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Failed to fork");
        }
        else if (pid > 0)
        {
            // Parent process
            if (!background)
            {
                waitpid(pid, NULL, 0); // Wait only for foreground processes
            }
            printf("parent picking back up\n");
        }
        else
        {
            // Child process
            // Prepare the arguments array for execv
            char **arguments = (char **)malloc((newSize + 2) * sizeof(char *)); // +2 for command and NULL terminator
            arguments[0] = firstString;                                         // First argument is the command itself
            for (int i = 1; i <= newSize; i++)
            {
                arguments[i] = tokens[i]; // Copy other arguments
            }
            arguments[newSize + 1] = NULL; // NULL terminate the array

            execv(firstString, arguments);
            perror("execv"); // execv only returns on error
            free(arguments);
            _exit(EXIT_FAILURE); // doing this bc the internet says I should!
        }
    }
    else
    {
        perror("Command not found or is not executable");
    }
}

/**
 * Searches for an executable in the current working directory and then in the PATH environment variable.
 * If found, it executes the command either in the foreground or background based on the user input.
 * Similar to mode1, it forks a child process for execution and waits for completion if not a background process.
 *
 * @param firstString The first token of the command, used to construct the full path of the executable.
 * @param tokens A NULL-terminated array of token strings representing the parsed user command.
 * @param newSize The number of tokens in the 'tokens' array excluding the background indicator '&' if present.
 */
void mode2(char *firstString, char **tokens, int newSize)
{
    char cwd[PATH_MAX];
    char fullPath[PATH_MAX];
    int found = 0; // Flag to indicate if the executable is found

    // Check if the last argument is '&' for background execution
    int background = 0;
    if (newSize > 0 && strcmp(tokens[newSize], "&") == 0)
    {
        background = 1;         // Mark as background execution
        tokens[newSize] = NULL; // Remove '&' from arguments for execv
        newSize--;              // Adjust size to exclude '&'
    }

    // First, try the current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", cwd, firstString); // Construct full path
        if (access(fullPath, F_OK | X_OK) == 0)
        {
            found = 1; // Executable found in current directory
        }
    }
    else
    {
        perror("getcwd() error");
    }

    // If not found, search the PATH directories
    if (!found)
    {
        char *pathEnv = getenv("PATH");
        if (pathEnv != NULL)
        {
            char *pathCopy = strdup(pathEnv); // Duplicate PATH to avoid modifying the original with strtok
            char *token = strtok(pathCopy, ":");

            while (token != NULL && !found)
            {
                snprintf(fullPath, sizeof(fullPath), "%s/%s", token, firstString); // Construct full path for each directory in PATH
                if (access(fullPath, F_OK | X_OK) == 0)
                {
                    found = 1; // Executable found in one of the PATH directories
                    break;     // Exit the loop as we found the executable
                }
                token = strtok(NULL, ":"); // Get next directory in PATH
            }
            free(pathCopy); // Free the duplicated PATH string
        }
    }

    // Execute if found
    if (found)
    {
        pid_t pid = fork(); // Fork process

        if (pid == -1)
        {
            perror("Failed to fork");
        }
        else if (pid > 0)
        {
            // Parent process
            if (!background)
            {
                waitpid(pid, NULL, 0); // waiting for the child to finish
            }
        }
        else
        {
            // Child process
            if (newSize > 0)
            {
                // Construct arguments array for execv
                char **arguments = malloc((newSize + 2) * sizeof(char *)); // +2 for command and NULL terminator
                arguments[0] = fullPath;                                   // First argument is the full path
                for (int i = 1; i <= newSize; i++)
                {
                    arguments[i] = tokens[i]; // Copy other arguments
                }
                arguments[newSize + 1] = NULL; // NULL-terminate the array

                execv(fullPath, arguments); // Execute the command
                perror("execv");            // If execv returns, it failed
                free(arguments);
                _exit(EXIT_FAILURE); // doing this bc the internet says I should!
            }
            else
            {
                char *args[] = {fullPath, NULL};
                execv(fullPath, args); // Execute without additional arguments
                _exit(EXIT_FAILURE);   // doing this bc the internet says I should!
            }
        }
    }
    else
    {
        // If the command is not found
        printf("Command not found\n");
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

/**
 * Trims leading and trailing whitespace from a string and returns a new string with
 * the trimmed content.
 *
 * @param str The string to be trimmed.
 * @return A newly allocated string containing the trimmed content of the input string.
 */

char *trim(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) // All spaces?
    {
        return strdup(""); // Return an empty string
    }

    // Trim trailing space. grabbed part of this from Geeks for Geeks
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator character
    end[1] = '\0';

    return strdup(str);
}

/**
 * Continuously prompts the user for input, parses the input into tokens, and processes the tokens
 * based on the command. The function ensures all dynamically allocated memory is properly freed to
 * avoid memory leaks.
 */
void prompt()
{
    while (1)
    {
        char *cmdline = (char *)malloc(MAXBUF);
        printf("dsh> ");
        fgets(cmdline, MAXBUF, stdin);
        cmdline[strlen(cmdline) - 1] = '\0'; // Remove the newline at the end

        char *trimmed_cmdline = trim(cmdline);
        if (strlen(trimmed_cmdline) == 0) // Check if the trimmed command is empty
        {
            free(trimmed_cmdline); // Free the trimmed command line
            free(cmdline);         // Free the original command line
            continue;              // Reprompt the user
        }

        char **terms = split(trimmed_cmdline, " ");
        configureMode(terms);

        // Free each dynamically allocated string in the terms array
        for (int i = 0; terms[i] != NULL; i++)
        {
            free(terms[i]);
        }
        free(terms); // Free the array itself

        free(trimmed_cmdline); // Free the trimmed command line
        free(cmdline);         // Free the original command line as well
    }
}
