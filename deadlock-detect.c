/*
  Operating Systems

  Prof. Wolffe

  Program 2


  Author: Doug MacDonald
 */

#include <stdio.h>
#include <stdlib.h>

#define debug 1

int main(int argc, char* argv[])
{
  int numberOfArgs = argc;
  char* filename;
  int processNumber;

  if(numberOfArgs < 3)
    {
      printf("Not enough arguments specified.\nPlease provide filename and process number.\n");
    }
  else
    {
      filename = argv[1];
      processNumber = atoi(argv[2]);
      if(debug){printf("Filename: %s\nProcessNumber: %d\n", filename, processNumber);}
    }

  return 0;
}
