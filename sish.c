#include <stdio.h>      // For printf(), perror(), getline()
#include <stdlib.h>     // For malloc(), free(), exit()
#include <string.h>     // For strcmp(), strtok()
#include <unistd.h>     // For fork(), execvp(), chdir(), getcwd()
#include <sys/types.h>  // For pid_t
#include <sys/wait.h>   // For wait(), waitpid()
#include <errno.h>      // For error handling with errno

// Part 1: The Shell
int main(int argc, char* argv[]){
    char *line = NULL; // buffer for storing input for getline()
    size_t len = 0;    // length of the line
    char *saveptr1;    // tracks strtok_r tokenizing commands + arguments
    char *saveptr2;    // tracks strtok_r tokenizing piped commands
    int pipeCommandCount = 0; // tracks the number of piped commands the user enters (for part 3)

    while(1) { // while shell is running
        printf("sish> "); // prompt
        ssize_t numOfChar = getline(&line, &len, stdin);  // get user input from stdin and store in line (buffer)

        if (numOfChar == -1) {  // Handling EOF error
            printf("\nExiting sish...\n");
            break;
        }
        
        if (strlen(line) == 0) // if user enters nothing, restart loop (reprompt the prompt)
            continue;  
        
        if (line[numOfChar-1] == '\n') 
            line[numOfChar-1] = '\0'; // remove newline to ensure matching with commands
        
        if (strcmp(line, "exit") == 0) 
            break; // exit if user enters exit
        
        
        // tokenize for pipes
        char *pipedToken = strtok_r(line, "|", &saveptr2);
        while (pipedToken != NULL) {
            pipeCommandCount++;

            // Tokenize command + argument(s)
            // create array to store the tokens, then use execvp to execute each one by one
            char *tok[100]; // max 100 tokens
            int tok_count = 0;
            char *token = strtok_r(pipedToken, " ", &saveptr1);  // tokenizing command + argument(s)

            while (token != NULL) {  // loop for storing command + its arguments in args array
                tok[tok_count++] = token;  
                token = strtok_r(NULL, " ", &saveptr1);  // move to the next token
            }
            tok[tok_count] = NULL;  // NULL terminate list

            // fork and execute the command
            pid_t pid = fork();
            if (pid == 0) {  // in child process
                execvp(tok[0], tok);
                perror("execvp failed"); // print error if execvp fails
                exit(1);  // terminates child process if execvp fails
            } else if (pid > 0) {  // in parent process
                waitpid(pid, NULL, 0);  // wait for child
            } else {
                perror("fork failed");
            }

           pipedToken = strtok_r(NULL, "|", &saveptr2); // go to next piped token
        }
        
    } // while shell loop

    free(line); // free allocated memory
    return 0;

} //main

// tokenizing command(s) entered by user
            // char *token = strtok_r(pipedToken, " ", &saveptr1);  // token for commands/arguments
            // while (token != NULL) {
            //     if (token[0] == '-') { 
            //         //this means the token is an argument to the command (ex ls -l, ls is the command and -l is the argument)
            //     }
            //     //Otherwise, this is the main command 
            //     //execute this token
            //     token = strtok_r(NULL, " ", &saveptr1);  // move to the next token (command/argument)