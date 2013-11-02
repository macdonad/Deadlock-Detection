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
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

//Prototypes
void read_file();
void handle_line(char*);
void handle_signal(int);
void debug_set();
void check_requested();
void owns_my_requested(char*);
void am_i_blocked();
void respond_to_probe(char*);
void *main_thread(void*);
void *sender_thread(void*);
void *receiver_thread(void*);
void set_up_smem();
void clean_and_exit();

//Define
#define MAX_BUF 1024

//Globals
int blocked = 0;
char* blockingProcess;
int processNumber;
char* processName;

char processes[10][3];
int processcount = 0;

int deadlocked = 0;
int debug = 0;
int sendprobes = 0;

char own[10][3];
int owncount = 0;
char request[10][3];
int requestcount = 0;

char blockedby[10][3];
int blockedbycount = 0;

char* filename;

char* sharedPtr;
int shmId;

//Main
int main(int argc, char* argv[])
{
  signal(SIGINT, handle_signal);

  debug_set();
  
  int numberOfArgs = argc;
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
  read_file();
  printf("Total Number of Processes: %d\n", processcount);


  //Print what I own
  if(debug)
    {
      int i = 0;
      while(i < owncount)
	{
	  printf("Own: %s\n", own[i]);
	  i++;
	}
    

  //Print What I am Requesting
      i = 0;
      while(i < requestcount)
	{
	  printf("Request: %s\n", request[i]);
	  i++;
	}
    }
  check_requested();

  set_up_smem();

  //Split into threads
  int thread = 0;
  //Main
  pthread_t main_thread_id;
  int mainthread = pthread_create(&main_thread_id, NULL, main_thread, (void *) &thread);
  if(debug){printf("Main Thread Id: %d\n", mainthread);}

  //Sender
  thread++;
  pthread_t sender_thread_id;
  int senderthread = pthread_create(&sender_thread_id, NULL, sender_thread, (void *) &thread);
  if(debug){printf("Sender Thread Id: %d\n", senderthread);}

  //Receiver
  thread++;
  pthread_t receiver_thread_id;
  int receiverthread = pthread_create(&receiver_thread_id, NULL, receiver_thread, (void *) &thread);
  if(debug){printf("Receiver Thread Id: %d\n", receiverthread);}

  pthread_exit(0);
}

void read_file()
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
  const char* s = " \n";
  char* piece;
  int me = 0;

  //owns = 0, request = 1
  int owns_request = 0;

  //Process
  piece = strtok(line, s);
  if(processcount == 0)
    {
      strcpy(processes[processcount], piece);
      processcount++;
    }
  else
    {
      int i = 0;
      int found = 0;
      while(i < processcount)
	{
	  if(!strcmp(piece, processes[i]))
	    {
	      found = 1;
	    }
	  i++;
	}
      if(!found)
	{
	  strcpy(processes[processcount], piece);
	  processcount++;
	}
    }

  if(!strcmp(piece, processName))
    {
      me = 1;
    }

  //Owns / Requests
  piece = strtok(NULL, s);
  if(me)
    {
      if(!strcmp(piece, "owns"))
	{
	  owns_request = 0;
	}
      else
	{
	  owns_request = 1;
	}
    }

  //Resource
  piece = strtok(NULL, s);
  if(me)
    {
      if(owns_request)
	{
	  strcpy(request[requestcount], piece);
	  requestcount++;
	}
      else
	{
	  strcpy(own[owncount], piece);	  
	  owncount++;
	}
    }  
  me = 0;
}

void handle_signal(int sigNum)
{
  printf("\nUggggghhhhhh...... Death. Process %s has died.\n", processName);
  clean_and_exit();
  exit(0);
}

void debug_set()
{
  char response;
  printf("Enable Debug?(Y/N)\n");
  scanf("%c", &response);
  if(response == 'Y' || response == 'y')
    {
      debug = 1;
    }
  else
    {
      debug = 0;
    }
}

void check_requested()
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
	  owns_my_requested(buffer);
	}
      while(c != EOF);
      fclose(file);
    }
  free(buffer);

  am_i_blocked();
}

void owns_my_requested(char* line)
{
  const char* s = " \n";
  char* piece;
  char* owningprocess;
  int i = 0;

  owningprocess = strtok(line, s);
  if(strcmp(owningprocess, processName))
    {
      piece = strtok(NULL, s);
      if(!strcmp(piece, "owns"))
	{
	  piece = strtok(NULL, s);
	    {
	      while(i < requestcount)
		{
		  if(!strcmp(piece, request[i]))
		    {		      
		      strcpy(blockedby[blockedbycount], owningprocess);
		      blockedbycount++;
		      //process is blocked
		      blocked = 1;
		    }
		    i++;
		}
	    }
	}
    }
}

void am_i_blocked()
{
  int i = 0;
  if(blocked)
    {
      printf("I'm Blocked\n");
      while(i < blockedbycount)
	{
	  if(debug){printf("Process: %s is blocked by process %s\n", processName, blockedby[i]);}
	  if(debug){printf("%d:%d:%c\n", processNumber, processNumber, blockedby[i][1]);}
	  i++;
	}
    }
  else
    {
      //not blocked
      printf("I'm Not Blocked\n");
    }
}


void *main_thread(void *arg)
{
  printf("Main Standing By\n");
  while(!deadlocked)
    {

    }
  //Kill process
  printf("Process: %d is Deadlocked\n", processNumber);
  pthread_exit(NULL);
}

void *receiver_thread(void *arg)
{
  printf("Receiver Standing By\n");
  char* probe = "init";
  while(1)
    {
      //initialization unnessecary delete after connection is up
      //read in probes

      if(probe != NULL)
	{

	  if(blocked)
	    {
	      //blocked
	      printf("Process: %s, read in %s, I am Blocked, Responding to Probe\n", processName, probe);
	      respond_to_probe(probe);
	      blocked = 0;
	    }
	  else
	    {
	      //Not blocked: Discard Probe
	      printf("Process: %s, read in %s, Not Blocked, Discarding...\n", processName, probe);
	      probe = NULL;
	    }
	}
    }
  pthread_exit(NULL);
}

//Send probe every 10 seconds unless deadlocked
void *sender_thread(void *arg)
{
  printf("Sender Standing By\n");
  while(blocked && !deadlocked)
    {
      //send probe every 10 seconds
    }

  if(deadlocked){printf("Stopping Probes, I am Deadlocked\n");}

  pthread_exit(NULL);
}

void respond_to_probe(char* probe)
{
  //Send response to probe
  printf("Sending probe response...\n");
}

void set_up_smem()
{
  // Set up shared Memory, every process will read it, then when all have read, someone can write.
  int totalProcesses = processcount;

  key_t mem_key = ftok("key", 0);
  pid_t my_pid = getpid();
  int size = 1024;

  //create shared mem
  if(processNumber == 1)
    {
      shmId = shmget(mem_key, size, IPC_CREAT|S_IRUSR|S_IWUSR); 
    }
  else
    {
      shmId = shmget(mem_key, size, S_IRUSR|S_IWUSR);
    }
  if(shmId < 0)
    {
      printf("Failed Shared Memory Create.\n");
      exit(-1);
    }

  //attach to shared mem
  sharedPtr = shmat(shmId, (void*)0, 0);
  if(sharedPtr < 0)
    {
      printf("failed to attach\n");
      clean_and_exit();
      exit(-1);
    }
}

void clean_and_exit()
{
  shmctl(shmId, IPC_RMID, 0);
  exit(0);
}
