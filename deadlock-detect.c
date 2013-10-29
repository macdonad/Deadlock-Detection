/*
  Operating Systems

  Prof. Wolffe

  Program 2


  Author: Doug MacDonald
 */

//Include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

//Debug value
#define debug 1


//Prototypes
void read_file(char*);
void handle_line(char*);
void handle_signal(int);

//Globals
int blocked = 0;
char* blockingProcess;
int processNumber;
char* processName;

//Main
int main(int argc, char* argv[])
{
  signal(SIGINT, handle_signal);

  int numberOfArgs = argc;
  char* filename;
  processName = argv[0];//Initialize

  if(numberOfArgs < 3)
    {
      printf("Not enough arguments specified.\nPlease provide filename and process number.\n");
    }
  else
    {
      filename = argv[1];
      processNumber = atoi(argv[2]);
      sprintf(processName, "P%d", processNumber);
      if(debug){printf("Filename: %s\nProcessNumber: %d\nProcessName: %s\n", filename, processNumber, processName);}
    }

  //read in file
  read_file(filename);


  return 0;
}

void read_file(char* filename)
{
  int size = 1024, pos;
  int c;
  char* buffer = (char*)malloc(size);

  FILE *file = fopen(filename, "r");
  if(file)
    {
      do
	{
	  pos = 0;
	  do
	    {
	      c = fgetc(file);
	      if(c != EOF) buffer[pos++] = (char)c;
	      if(pos >= size - 1)
		{
		  size *= 2;
		  buffer = (char*)realloc(buffer, size);
		}
	    }
	  while(c != EOF && c != '\n');
	  buffer[pos] = 0;
	  handle_line(buffer);
	}
      while(c != EOF);
      fclose(file);
    }
  free(buffer);
}

//Takes line from text and checks 
void handle_line(char* line)
{
  const char* s = " ";
  char* piece;
  int me = 0;

  //Process
  piece = strtok(line, s);
  printf("%s\n", piece);
  if(!strcmp(piece, processName))
    {
      me = 1;
      printf("ME!\n");
    }

  //Owns / Requests
  piece = strtok(NULL, s);
  printf("%s\n", piece);
  if(me)
    {
      if(!strcmp(piece, "owns"))
	{
	  printf("I own something!\n");
	}
      else
	{
	  printf("gimme!\n");
	  //Need to check if this is owned by someone
	  //Then send my messages to that process
	}
    }

  //Resource
  piece = strtok(NULL, s);
  printf("%s\n", piece);
  if(me)
    {
      printf("THIS! %s\n", piece);
    }
}

void handle_signal(int sigNum)
{
  printf("\nUggggghhhhhh...... Death.\n");
  exit(0);
}
