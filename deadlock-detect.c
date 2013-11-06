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
#include <semaphore.h>

//Prototypes
void read_file();
void handle_line(char*);
void handle_signal(int);
void debug_set();
void check_requested();
void owns_my_requested(char*);
void am_i_blocked();
void send_probe(char*);
void *main_thread(void*);
void *sender_thread(void*);
void *receiver_thread(void*);
void set_up_smem();
void clean_and_exit();

//Define
#define MAX_BUF 1024

//Shared Semaphore to protect shared memory
#define SNAME "protector"
#define DEAD "deadlock"

//Globals
int blocked = 0;
char* blockingProcess;
int processNumber;
char* processName;

//Thread id's
pthread_t main_thread_id;
pthread_t receiver_thread_id;
pthread_t sender_thread_id;

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

char* splitter = "#\n";

int messageId = -1;

//Semaphores
sem_t sender;
sem_t getMessageId;
sem_t* protector;
sem_t* deadlock;

//Main
int main(int argc, char* argv[])
{
  //Create Signal Handler
  signal(SIGINT, handle_signal);

  //Prompt for debug
  debug_set();
  
  int numberOfArgs = argc;
  processName = argv[0];//Initialize

  //Verify Args
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
  if(debug){printf("Total Number of Processes: %d\n", processcount);}


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

  //Check the processes I am requesting
  check_requested();

  //Set up shared memory
  set_up_smem();

  //Init process specific semaphores
  sem_init(&sender, 0, 1);
  sem_init(&getMessageId, 0, 1);

  //Split into threads
  int thread = 0;
  //Main
  int mainthread = pthread_create(&main_thread_id, NULL, main_thread, (void *) &thread);
  if(mainthread < 0)
    {
      printf("Error setting up main thread.\n");
    }

  //Sender
  thread++;
  int senderthread = pthread_create(&sender_thread_id, NULL, sender_thread, (void *) &thread);
  if(senderthread < 0)
    {
      printf("Error setting up sender thread.\n");
    }
  //Receiver
  thread++;
  int receiverthread = pthread_create(&receiver_thread_id, NULL, receiver_thread, (void *) &thread);
  if(debug){printf("Receiver Thread Id: %d\n", receiverthread);}
  if(receiverthread < 0)
    {
      printf("Error setting up receiver thread.\n");
    }

  //Only end when all threads are done
  pthread_exit(0);
}


//Read file
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

//Signal Handler
void handle_signal(int sigNum)
{
  printf("\nUggggghhhhhh...... Death. Process %s has died.\n", processName);
  clean_and_exit();
  exit(0);
}

//Prompt for Debug
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

//Check the processes I am requesting for their owning process
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

//Check who owns my requests
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

//Check if I am blocked
void am_i_blocked()
{
  int i = 0;
  if(blocked)
    {
      printf("I'm Blocked\n");
      if(debug)
	{
	  while(i < blockedbycount)
	    {
	      printf("Process: %s is blocked by process %s\n", processName, blockedby[i]);
	      printf("%d:%d:%c\n", processNumber, processNumber, blockedby[i][1]);
	      i++;
	    }
	}
    }
  else
    {
      //not blocked
      printf("I'm Not Blocked\n");
    }
}


//Main thread, do nothing unless deadlocked
void *main_thread(void *arg)
{
  //wait for detection of deadlock
  sem_wait(deadlock);
  
  //pass detection on
  sem_post(deadlock);

  //Kill process
  int pid = getpid();
  printf("***Process: %s (PID: %d) is Deadlocked.***\n", processName, pid);

  pthread_cancel(sender_thread_id);
  pthread_cancel(receiver_thread_id);
  clean_and_exit();
  pthread_exit(NULL);
}

//Continually check shared memory
void *receiver_thread(void *arg)
{
  //Message Id, Read Count, Message ( Blocked Process :  Sender : Receiver)
  char* probe;
  int readcount;
  char* message;
  char* temp;
  int newmessageid;
  while(1)
    {
      //read in probes
      
      sem_wait(protector);
      sem_wait(&getMessageId);

      temp = (char*)malloc(strlen(sharedPtr) + 1);
      strcpy(temp, sharedPtr);
      newmessageid = atoi(strtok(temp, splitter));

      sem_post(&getMessageId);
      sem_post(protector);

      //Have I read this before?
      if(messageId < newmessageid)
	{
	  free(temp);

	  sem_wait(&getMessageId);

	  messageId = newmessageid;

	  sem_post(&getMessageId);

	  sem_wait(protector);

	  temp = (char*)malloc(strlen(sharedPtr) + 1);
	  strcpy(temp, sharedPtr);
	  newmessageid = atoi(strtok(temp, splitter));

	  //inc read count
	  readcount = atoi(strtok(NULL, splitter));
	  readcount++;

	  //0:0:0
	  message = strtok(NULL, splitter);

	  //Put shared memory back with inc read count
	  sprintf(sharedPtr, "%d#%d#%s", newmessageid, readcount, message);

	  probe = (char*)malloc(strlen(sharedPtr) + 1);
	  strcpy(probe, sharedPtr);

	  sem_post(protector);

	  //Not deadlocked, valid message, o ahread and process
	  if(probe != NULL && newmessageid != 0 && !deadlocked)
	    {
	      if((message[4]-'0') == processNumber)
		{
		  //blocked
		  //MessageId#ReadCount#Message(BlockedProcess:Sender:Receiver)
		  //0#0#1:2:3
		  
		  if(blocked)
		    {
		      if((message[0] - '0') == processNumber)
			{
			  //deadlocked
			  if(debug){printf("Set Deadlocked flag\n");}
			  deadlocked = 1;
			  sem_post(deadlock);
			}
		      //Not deadlocked
		      sem_wait(&getMessageId);

		      sprintf(probe, "%d#%d#%c:%d:%c", messageId + 1, 0, message[0], processNumber, blockedby[0][1]);

		      sem_post(&getMessageId);

		      send_probe(probe);			 
		    }
		  else
		    {
		      //Not blocked: Discard Probe
		      int pid = getpid();
		      printf("Process: %s (PID: %d) read in %s, Not Blocked, Discarding...\n", processName, pid, message);
		      probe = NULL;
		    }
		}
	      free(temp);
	      free(probe);
	    }
	}
    }
  pthread_exit(NULL);
}

//Send probe every 10 seconds unless deadlocked
void *sender_thread(void *arg)
{
  while(blocked && !deadlocked)
    {
      //send probe every 10 seconds
      sleep(10);

      //Send a probe for each process blocking me
      char probe[10];
      int i = 0;
      while(i < blockedbycount)
	{
	  sem_wait(&getMessageId);

	  //write probe to shared memory
	  sprintf(probe, "%d#%d#%d:%d:%c\n", messageId + 1, 0, processNumber, processNumber, blockedby[i][1]);

	  sem_post(&getMessageId);

	  send_probe(probe);
	  i++;
	}
    }

  pthread_exit(NULL);
}

//Send a message
void send_probe(char* probe)
{
  sem_wait(&sender);
  int readcount;
  int newmessageid;
  char* message;
  char* temp;

  sem_wait(protector);

  temp = (char*)malloc(strlen(sharedPtr) + 1);
  strcpy(temp, sharedPtr);
  newmessageid = atoi(strtok(temp, splitter));
  readcount = atoi(strtok(NULL, splitter));
  message = strtok(NULL, splitter);

  sem_post(protector);

  //Make sure everyone read it
  while(readcount < processcount)
    {
      free(temp);
      
      sem_wait(protector);

      temp = (char*)malloc(strlen(sharedPtr) + 1);
      strcpy(temp, sharedPtr);
      newmessageid = atoi(strtok(temp, splitter));
      readcount = atoi(strtok(NULL, splitter));
      message = strtok(NULL, splitter);        

      sem_post(protector);
    }

  free(temp);

  //Send response to probe
  sprintf(sharedPtr, "%s", probe);
  int pid = getpid();

  strtok(probe, splitter);
  strtok(NULL, splitter);
  char* third = strtok(NULL, splitter);

  if(blocked)
    {
      if(deadlocked)
	{
	  printf("Process: %s (PID: %d), is deadlocked.\n", processName, pid);
	}
      else
	{
	  printf("Process: %s (PID: %d), is blocked, sent a new probe: %s.\n", processName, pid, third);
	}
    }
  else
    {
      printf("Process: %s (PID: %d), is not blocked, sent a new probe: %s.\n", processName, pid, third);
    }

  sem_post(&sender);
}

//Set up shared mem, and semaphore to protect it
void set_up_smem()
{
  // Set up shared Memory, every process will read it, then when all have read, someone can write.
  
  key_t mem_key = ftok("key", 0);
  int size = 1024;
  int creator = 0;

  //create shared mem
  //First Process to get here will fail then create mem, other processes won't fail and will not create more shared mem.
  shmId = shmget(mem_key, size, S_IRUSR|S_IWUSR);
  if(shmId < 0)
    {
       shmId = shmget(mem_key, size, IPC_CREAT|S_IRUSR|S_IWUSR); 
       printf("Shared Memory created\n");
       creator = 1;
    }
  else
    {
      printf("Attached to shared memory\n");
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
  if(creator)
    {
      sprintf(sharedPtr, "%d#%d#%d:%d:%c", 0,0,0,0,'0');
      if(debug){printf("Init Message: %s\n", sharedPtr);}
      
      //Create shared semaphore
      printf("Created shared semaphore\n\n");
      protector = sem_open(SNAME, O_CREAT, 0644, 1);
      sem_init(protector, 1, 1);

      deadlock = sem_open(DEAD, O_CREAT, 0644, 1);
      sem_init(deadlock, 1, 0);
    }
  else
    {
      //Attach to shared memory
      printf("Attached to shared semaphore\n\n");
      protector = sem_open(SNAME, 0);
      deadlock = sem_open(DEAD, 0);
    }
}

void clean_and_exit()
{
  //Clean up and exit
  shmctl(shmId, IPC_RMID, 0);
  exit(0);
}
