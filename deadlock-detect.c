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

//Prototypes
void read_file(char*);
void handle_line(char*);
void handle_signal(int);
void debug_set();

//Globals
int blocked = 0;
char* blockingProcess;
int processNumber;
char* processName;

int debug = 0;

char* own[3];
int owncount = 0;
char* request[3];
int requestcount = 0;

//Main
int main(int argc, char* argv[])
{
  signal(SIGINT, handle_signal);

  debug_set();

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

  while(owncount >= 0)
    {
      printf("Own: %s\n", own[owncount]);
      owncount--;
    }
  while(requestcount >= 0)
    {
      printf("Request: %s\n", request[requestcount]);
      requestcount--;
    }


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

  //owns = 0, request = 1
  int owns_request = 0;

  //Process
  piece = strtok(line, s);
  if(debug){printf("%s\n", piece);}
  if(!strcmp(piece, processName))
    {
      me = 1;
      if(debug){printf("ME!\n");}
    }

  //Owns / Requests
  piece = strtok(NULL, s);
  if(debug){printf("%s\n", piece);}
  if(me)
    {
      if(!strcmp(piece, "owns"))
	{
	  if(debug){printf("I own something!\n");}
	  owns_request = 0;
	}
      else
	{
	  if(debug){printf("gimme!\n");}
	  owns_request = 1;
	  //Need to check if this is owned by someone
	  //Then send my messages to that process
	}
    }

  //Resource
  piece = strtok(NULL, s);
  if(debug){printf("%s\n", piece);}
  if(me)
    {
      if(debug){printf("THIS! %s\n", piece);}
      if(owns_request)
	{
	  request[requestcount] = piece;
	  requestcount++;
	}
      else
	{
	  own[owncount] = piece;
	  owncount++;
	}
    }  
}

void handle_signal(int sigNum)
{
  printf("\nUggggghhhhhh...... Death.\n");
  exit(0);
}

void debug_set()
{
  char response;
  printf("Enable Debug?(Y/N)\n");
  scanf("%c", &response);
  if(response == 'Y'||response == 'y')
    {
      debug = 1;
    }
  else
    {
      debug = 0;
    }
}
