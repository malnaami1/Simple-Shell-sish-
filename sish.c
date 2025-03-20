#include <stdio.h>      // For printf(), perror(), getline()
#include <stdlib.h>     // For malloc(), free(), exit()
#include <string.h>     // For strcmp(), strtok()
#include <unistd.h>     // For fork(), execvp(), chdir(), getcwd()
#include <sys/types.h>  // For pid_t
#include <sys/wait.h>   // For wait(), waitpid()
#include <errno.h>      // For error handling with errno


void tokenize_command(char *command, char *tok[], int *tok_count);

// Part 1: The Shell
int main(int argc, char* argv[]){
    char *line = NULL; // buffer for storing input for getline()
    size_t len = 0;    // length of the line

    // array to store history
    char *history[100];
    int hist_count = 0;

    while(1) { // while shell is running
        printf("sish> "); // prompt
        ssize_t numOfChar = getline(&line, &len, stdin);  // get user input from stdin and store in line (buffer)

        if (numOfChar == -1) {  // Handling EOF/getline errors
            perror("Getline failed");
            printf("\nExiting sish...\n");
            break;
        }
        
        if (strlen(line) == 0) // if user enters nothing, restart loop (reprompt)
            continue;  
        
        if (line[numOfChar-1] == '\n') 
            line[numOfChar-1] = '\0'; // remove newline to ensure matching with commands
        
        if (strcmp(line, "exit") == 0)
            break; // exit if user enters exit


        // store history of the last 100 commands
        char *input = strdup(line);  // duplicate user entry
        if (input == NULL) { // handle memory allocation failure
            perror("Strdup failed");
        }

        if (hist_count < 100) {
            history[hist_count++] = input; // store user input in array
        } else {
            free(history[0]); // get rid of first element of array to make space for new ones
            for(int i = 1; i < 100; i++) {
                history[i-1] = history[i];  // shift elements to the left
            }
            history[99] = input;  // store latest user input in last element
            hist_count++;
        }

        // check for piped commands
        char *saveptr2; 
        char *pipedToken = strtok_r(line, "|", &saveptr2);
        while (pipedToken != NULL) {
            int pipeCommandCount = 0; // tracks the number of piped commands the user enter
            pipeCommandCount++;

            // create array to store the tokens, then use execvp to execute each one by one
            char *tok[100]; // max 100 tokens
            int tok_count = 0;
            tokenize_command(pipedToken, tok, &tok_count); // function for tokenizing commands and their arguments

            // check for cd command
            if (strcmp(tok[0], "cd") == 0) {
                if (tok[1] == NULL) {
                    fprintf(stderr, "Error: missing argument.\n"); // if no arguments provided, print error
                } else {
                    if (chdir(tok[1]) != 0) { // change directory using chdir system call
                        perror("cd");  // print error msg (eg no such directory) if chdir doesnt return 0 (fails)
                    }
                }
                pipedToken = strtok_r(NULL, "|", &saveptr2); // go to next piped token
                continue;  // skip forking if cd is run
            }

            // check for history command
            if (strcmp(tok[0], "history") == 0) {
                if (tok[1] == NULL) {  // if no arguments alongside history...
                    int i = 0;
                    while (i != hist_count){
                        printf("%d %s\n", i, history[i]);  //...then print last 100 commands
                        i++;
                    }
                } else if (strcmp(tok[1], "-c") == 0){  // if argument is -c (clear)...
                    int i = 0;
                    while (i != hist_count){
                        free(history[i++]);   // ...delete every element in array
                    }
                    hist_count = 0;
                } else { // if the argument is an offset (eg 30, 67, etc)...
                    // ...execute the command at said offset 
                        int offset = atoi(tok[1]);  // convert the argument to an integer
                        if (offset >= 0 && offset <= hist_count) {
                        printf("%d %s\n", offset, history[offset]);  // display command with given offset
                        
                        // must tokenize to use execvp since history stores commands as single strings
                        char *tok2[100];
                        int tok2_count = 0;

                        // tokenize command using the function
                        tokenize_command(history[offset], tok2, &tok2_count);

                        pid_t pid2 = fork(); // fork and execute the command
                        if (pid2 == 0) {  // in child process
                            execvp(tok2[0], tok2);
                            perror("Execvp failed"); // print error if execvp fails
                            exit(1);  // terminate child process if execvp fails
                        } else if (pid2 > 0) {  // in parent process
                            waitpid(pid2, NULL, 0);  // wait for child 
                        } else {
                            perror("Fork failed");
                        }
                    } else {
                        fprintf(stderr, "Invalid offset\n");
                    }

                }
                pipedToken = strtok_r(NULL, "|", &saveptr2); // go to next piped token
                continue;  // skip forking if history is run
            }

            // fork and execute the command
            pid_t pid = fork();
            if (pid == 0) {  // in child process
                execvp(tok[0], tok);
                perror("Execvp failed"); // print error if execvp fails
                exit(1);  // terminates child process if execvp fails
            } else if (pid > 0) {  // in parent process
                waitpid(pid, NULL, 0);  // wait for child
            } else {
                perror("Fork failed");
            }

           pipedToken = strtok_r(NULL, "|", &saveptr2); // go to next "piped" token
        }
        
    } //while shell loop

    free(line); // free allocated memory
    return 0;

} //main


// Tokenize command + argument(s)
void tokenize_command(char *command, char *tok[], int *tok_count) {
    char *saveptr;   // tracks strtok_r tokenizing commands + arguments
    char *token = strtok_r(command, " ", &saveptr); 

    while (token != NULL) {  // loop for storing command + its arguments in tok array
        tok[(*tok_count)++] = token;  // populate tok array with the token
        token = strtok_r(NULL, " ", &saveptr); // move to the next token
    }
    tok[*tok_count] = NULL;  // NULL terminate list
}
        