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
int executeCommandPrepare(char** args, int argc, char* rmessage, int rcvid);
int executeCommandBattle(char** args, int argc, char* rmessage, int rcvid);

char message[512];

int rcvid1;
int rcvid2;
int playersConnected = 0;
int finished = 0;

int** player1Ships;
int** player2Ships;

int* p1Shot;
int* p2Shot;

int p1Hit;
int p2Hit;

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

	player1Ships = (int**) malloc(sizeof(int*) * 5);
	player2Ships = (int**) malloc(sizeof(int*) * 5);

	p1Shot = (int*) malloc(sizeof(int*) * 2);
	p2Shot = (int*) malloc(sizeof(int*) * 2);

	while(1)
	{
		// Preparing
		while(1)
		{
			int rcvid = MsgReceive(chid,message,sizeof(message),NULL);
			printf("Message received, rcvid %X: \"%s\"\n",rcvid,message);
			int n; char **spl = split(message,&n,10);
			int done = executeCommandPrepare(spl,n,message, rcvid);
			freeChar2D(spl, n);
			if(done == 1) break;
		}
		strcpy(message,"Player 1");
		MsgReply(rcvid1,EOK,message,sizeof(message));
		strcpy(message,"Player 2");
		MsgReply(rcvid2,EOK,message,sizeof(message));

		// Battle
		while(1)
		{
			int i = 0;
			while(1 < 2)
			{
				// recieve
				int rcvid = MsgReceive(chid,message,sizeof(message),NULL);
				printf("Message received, rcvid %X: \"%s\"\n",rcvid,message);
				if(rcvid != rcvid1 && rcvid != rcvid2)
				{
					strcpy(message,"No");
					MsgReply(rcvid,EOK,message,sizeof(message));
				}
				else
				{
					int n; char **spl = split(message,&n,10);
					int done = executeCommandBattle(spl,n,message, rcvid);
					freeChar2D(spl, n);
					if(done == 1) break;
				}
				i++;
			}
			sprintf(message,"%d %d%d %d %d%d %d",p1Hit,p1Shot[0],p1Shot[1],p2Hit,p2Shot[0],p2Shot[1],finished);
			printf("%s",message);
			MsgReply(rcvid1,EOK,message,sizeof(message));
			sprintf(message,"%d %d%d %d %d%d %d",p2Hit,p2Shot[0],p2Shot[1],p1Hit,p1Shot[0],p1Shot[1],finished);
			printf("%s",message);
			MsgReply(rcvid2,EOK,message,sizeof(message));

			if(finished == 1 || finished == 2) break;
		}

		// NULLfy everything
		playersConnected = 0;
	}

	return 0;
}


int executeCommandPrepare(char** args, int argc, char* rmessage, int rcvid)
{
	if(strcmp(args[0],"Ready") == 0)
	{
		int i;
		if(playersConnected == 0)
		{
			rcvid1 = rcvid;
			playersConnected++;
			for(i = 0; i < 5; i++)
			{
				int* cell = (int*) malloc(sizeof(int) * 2);
				cell[0] = args[i+1][0] - '0';
				cell[1] = args[i+1][1] - '0';
				player1Ships[i] = cell;
			}
			return 0;
		}
		else if(playersConnected == 1)
		{
			rcvid2 = rcvid;
			playersConnected++;
			for(i = 0; i < 5; i++)
			{
				int* cell = (int*) malloc(sizeof(int) * 2);
				cell[0] = args[i+1][0] - '0';
				cell[1] = args[i+1][1] - '0';
				player2Ships[i] = cell;
			}
			return 1;
		}
	}
	return 0;
}

int executeCommandBattle(char** args, int argc, char* rmessage, int rcvid)
{
	int* cell = (int*) malloc(sizeof(int) * 2);
	cell[0] = args[0][0] - '0';
	cell[1] = args[0][1] - '0';

	int i;

	if(rcvid == rcvid1)
	{
		p1Shot = cell;
		for(i = 0; i < 5; i++)
		{
			if(player2Ships != NULL)
			{
				if(player2Ships[i][0] == p1Shot[0] && player2Ships[i][1] == p1Shot[1])
				{
					p1Hit = 1;
					player2Ships[i] = NULL;
				}
			}
		}
	}
	else if(rcvid == rcvid2)
	{
		p2Shot = cell;
		for(i = 0; i < 5; i++)
		{
			if(player1Ships != NULL)
			{
				if(player1Ships[i][0] == p2Shot[0] && player1Ships[i][1] == p2Shot[1])
				{
					p2Hit = 1;
					player1Ships[i] = NULL;
				}
			}
		}
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

void freeChar2D(char** arr, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		free(arr[i]);
	}
	free(arr);
}
