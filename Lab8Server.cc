#include <sys/iomsg.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/neutrino.h>
#include <process.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/netmgr.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>

int chid;

char** split(char* string, int* finalSize, int splitMaxSize);
void freeChar2D(char** arr, int size);
void executeCommand(char** args, int argc, char* rmessage);
void listFile(FILE* f, char* string);
void listDirectory(DIR* d, char* string);

char message[512];
int rcvid;

int main()
{
	// Create channel and save info to file
	chid = ChannelCreate(0);
	pid_t thisPid = getpid();

	size_t ndStringSize = 100;
	char* ndString = (char*) malloc(ndStringSize * sizeof(char));
	netmgr_ndtostr(ND2S_DIR_SHOW | ND2S_NAME_SHOW | ND2S_DOMAIN_SHOW, ND_LOCAL_NODE, ndString, ndStringSize);

	char *filename = "serverInfo.txt";
	FILE *f = fopen(filename,"r");
	if(f)
	{
		fclose(f);
		remove(filename);
	}
	f = fopen(filename,"w");
	fprintf(f,"%s\n%d\n%d",ndString,thisPid,chid);
	fclose(f);
	free(ndString);

	// Receiver
	while(1)
	{
		// recieve
		rcvid = MsgReceive(chid,message,sizeof(message),NULL);
		printf("Message received, rcvid %X: \"%s\"\n",rcvid,message);

		// parse and execute commands
		if(strlen(message) > 0)
		{
			// parse reply
			int n;
			char **spl = split(message,&n,10);

			// form command
			executeCommand(spl,n,message);

			// free memory
			freeChar2D(spl, n);
		}
		else
		{
			// default answer
			strcpy(message,"You have not entered anything");
		}

		// reply
		MsgReply(rcvid,EOK,message,sizeof(message));
	}
	return 0;
}

char** split(char* string, int* finalSize, int splitMaxSize)
{
	char *copy = strdup(string);
	char **spl = (char**) malloc(sizeof(char*) * 512);
	int n = 0;
	char *single = strtok_r(copy," ",&string);
	while(single != NULL)
	{
		//realloc(spl,(n+1) * sizeof(char*));
		spl[n] = strdup(single);
		single = strtok_r(NULL," ",&string);
		n++;
	}
	free(single);
	*finalSize = n;
	return spl;
}

void executeCommand(char** args, int argc, char* rmessage)
{
		strcpy(rmessage,"Unknown command");
}

void freeChar2D(char** arr, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		free(arr[i]);
	}
	free(arr);
}
