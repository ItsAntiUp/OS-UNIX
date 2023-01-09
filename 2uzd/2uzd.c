/*Programming OS UNIX task 2.
Author: Kostas Ragauskas, INF course 3, group 2.
*/

//TODO: split line by delimiter with multiple chars
//TODO: pipe with access.log fails (maybe file is too big?)
//TODO: some specific commands wont work (cd, ./Main etc)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_BUFFER 1024
#define MAX_COMMAND_BUFFER 32

//Function to read input from the user
char* readUserInput(char* message) {
   int bufferSize = MAX_BUFFER;
   int len = 0;
   int chr;

   char* line = (char*)malloc(sizeof(char) * bufferSize);
   if(!line){
      fprintf(stderr, "Memory allocation failed!");
      exit(-1);
   }

   printf("\n%s", message);

   while(1) {
      //If we detect 'enter' or an end of file - end the input, else - append new char
      if((chr = getchar()) == '\n') {
         line[len++] = '\0';
         break;
      }
      else if(chr == EOF) {
         line = NULL;
         break;
      }
      else
         line[len++] = chr;

      //If we have exceeded the buffer, reallocate memory
      if(len >= bufferSize) {
         bufferSize += MAX_BUFFER;

         line = realloc(line, sizeof(char) * bufferSize);
         if(!line) {
            fprintf(stderr, "Memory allocation failed!");
            exit(-1);
         }
      }
   }

   return line;
}

//Function to split a given line into multiple
int splitLine(char* line, char** splitLine, char* delimiter) {
   int bufferSize = MAX_COMMAND_BUFFER;
   int numberOfCommands = 0;
   char* token;
   
   //While token is not null, append it to our array
   for(token = strtok(line, delimiter); token; token = strtok(NULL, delimiter)){
      splitLine[numberOfCommands++] = strdup(token);

      //If we have exceeded the buffer, reallocate memory
      if(numberOfCommands >= bufferSize) {
         bufferSize += MAX_COMMAND_BUFFER;

         splitLine[numberOfCommands] = realloc(splitLine[numberOfCommands], sizeof(char*) * bufferSize);
         if(!splitLine[numberOfCommands]) {
            fprintf(stderr, "Memory allocation failed!");
            exit(-1);
         }
      }
   }

   return numberOfCommands;
}

int forkAndExec(char** arguments, int FDin, int FDout) {
   pid_t pid = fork();
   if (pid < 0) {
      fprintf(stderr, "Fork failed!");
      return -1;
   }

   //If we are in the parent process - wait for it to finish and exit
   if (pid) {
      //wait(NULL);
      return pid;
   }

   //If we are in the child process and FDin is not -1 nor 0 (standard input) - getting input from pipe
   if(FDin != -1 && FDin != 0) {
      //Creating a new file descriptor (STDIN), which points to FDin (reading from pipe)
      dup2(FDin, STDIN_FILENO);
      close(FDin);
   }

   //If we are in the child process and FDout is not -1 nor 1 (standard output)- passing output to the pipe 
   if(FDout != -1 && FDout != 1) {
      //Creating a new file descriptor (STDOUT), which points to FDout (writing to pipe)
      dup2(FDout, STDOUT_FILENO);
      close(FDout);
   }

   //Executing the program
   if(execve(arguments[0], arguments, NULL) < 0){
      fprintf(stderr, "Error while executing command!");
      return -1;
   }

   //Waiting for child processes to finish
   //waitpid(pid, NULL, 0);
   return pid;
}

void runPipeline(int pipeNumber, int* commandNumbers, char **splitCommands[], int pids[]) {
   //Don't change stdin in the beginning (FDin = -1)
   int FDin = -1, FDout;

   for(int i = 0; i < pipeNumber; i++) {
      //File descriptors for the pipe
      int FDpipe[2];

      //if this is not the last command, set up a pipe for stdout (set FDout to be writing to pipe)
      if(i + 1 < pipeNumber) {
         if(pipe(FDpipe) == -1) {
            fprintf(stderr, "Pipe failed!");
            break;
         }

         FDout = FDpipe[1];
      }

      //Otherwise, don't change stdout (writing output to the screen)
      else
         FDout = -1;

      //Parsing the split lines into appropriate arguments
      char* arguments[commandNumbers[i] + 1];
      arguments[0] = strdup("/bin/");
      strcat(arguments[0], splitCommands[i][0]);

      for(int j = 1; j < commandNumbers[i]; ++j)
         arguments[j] = strdup(splitCommands[i][j]);

      arguments[commandNumbers[i]] = NULL;

      //Fork and run child with given arguments
      pids[i] = forkAndExec(arguments, FDin, FDout);

      //Closing and freeing the memory
      close(FDin);
      close(FDout);

      for(int j = 0; j < commandNumbers[i] + 1; ++j)
         free(arguments[j]);

      if(pids[i] == -1)
         break;

      //set up stdin for next command (FDin is now reading from pipe)
      FDin = FDpipe[0]; 
   }

   for(int i = 0; i < pipeNumber; ++i) {
      waitpid(pids[i], NULL, 0);
   }
}

void runLoop(){
   while(1) {
      char* line = readUserInput("> ");
      
      //Checking if line is EOF or user inputed "exit"
      if(line == NULL || strcmp(line, "exit") == 0) {
         printf("Goodbye!\n");
         free(line);
         exit(1);
      }
      else {
         //Creating a string for split commands (by "|")
         char** splitPipes = (char**)malloc(sizeof(char*) * MAX_COMMAND_BUFFER);
         if(!splitPipes) {
            fprintf(stderr, "Memory allocation failed!");
            free(line);
            break;
         }

         //Splitting the line by "|" and getting the command number
         char* pipeDelimiter = "|";
         int pipeNumber = splitLine(line, splitPipes, pipeDelimiter);
         int* commandNumbers = (int*)malloc(sizeof(int) * MAX_COMMAND_BUFFER);
         if(!commandNumbers) {
            fprintf(stderr, "Memory allocation failed!");
            free(splitPipes);
            free(line);
            break;
         }

         //Splitting each pipe command by spaces, tabs and carriage returns
         char** splitCommands[pipeNumber];
         char* commandDelimiter = " \t\r";

         for(int i = 0; i < pipeNumber; ++i){
            splitCommands[i] = (char**)malloc(sizeof(char*) * MAX_COMMAND_BUFFER);
            if(!splitCommands[i]) {
               fprintf(stderr, "Memory allocation failed!");
               free(splitPipes);
               free(commandNumbers);
               free(line);
               break;
            }

            commandNumbers[i] = splitLine(splitPipes[i], splitCommands[i], commandDelimiter);
         }

         /*if(strcmp(splitCommands[0][0], "cd") == 0)
            chdir(splitCommands[0][1]);
         else{*/
            //Running the commands one by one, piping one's output to the other's input
            int pids[MAX_BUFFER];
            runPipeline(pipeNumber, commandNumbers, splitCommands, pids);
         //}

         for(int i = 0; i < pipeNumber; ++i)
            free(splitCommands[i]);

         free(splitPipes);
         free(commandNumbers);
      }

      free(line);
   }
}

int main(int argc, char* argv[]) {
   runLoop();
   return 0;
}
