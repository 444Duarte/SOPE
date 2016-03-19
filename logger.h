#ifndef LOGGER_H_ 
#define LOGGER_H_

#include <stdio.h> 
#include <unistd.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <dirent.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/wait.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <libgen.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h> 
#include <semaphore.h>

#define SHARED_PROCESS 1
#define DEFAULT_STRING_SIZE 200
#define PROJ_ID 'a'
#define MEMPATH "/tmp"
#define FIMDEATENDIMENTO "fim_atendimento"
#define FIMDEATENDIMENTOSIZE 15
#define SHM_SIZE 4086

//Shared Memory index

#define OPENINGTIME 0
#define NUMOFBALCOES 1
#define NUMOFOPENBALCOES 2
#define NUMOFACTIVEBALCOES 3
#define NUMBEROFCLIENTS 4
#define BALCAODEFINING 5
#define NUMOFBALCAOVARIABLES 7

//Log events

#define SHMEMINIT 0
#define BALCAOFIFOCREATION 1
#define CLIASKSSERVICE 2
#define BALCAOSTARTSSERVICE 3
#define BALCAOSTOPSSERVICE 4
#define CLIENTENDSERVICE 5
#define CLOSEBALCAO 6
#define BALCAOSTOPSSERVING 7
#define STORECLOSE 8

//Log Senders

#define BALCAO 0
#define CLIENT 1

//Struct

struct balcaoData{

int timeIsOpen ;
int numberBalcao;
int *pt;
sem_t *sem;
char* shmemName;
int clientOnLine;
};

struct atendimentoData{
struct balcaoData *argStruct;
char clientFIFOID[DEFAULT_STRING_SIZE];
};

/*Function to write to log files*/
void writeLOG(char* shmemName,time_t currTime,int sender,int numberofBalcao, int event, char* channelUsed){
	char toWrite[DEFAULT_STRING_SIZE];
	char timeStr[DEFAULT_STRING_SIZE];
	strcpy(timeStr,ctime(&currTime));
	timeStr[strlen(timeStr) - 1] = '\0';
	sprintf(toWrite,"%-20s|",timeStr);
	if(sender == 0)
		sprintf(toWrite + strlen(toWrite)," Balcao |");
	else sprintf(toWrite + strlen(toWrite)," Cliente|");
	sprintf(toWrite + strlen(toWrite)," %-6d |", numberofBalcao);
	switch(event){
		case SHMEMINIT: 
			sprintf(toWrite + strlen(toWrite)," %30s |", "Criada MemPartilhada");
			break;
		case BALCAOFIFOCREATION:
			sprintf(toWrite + strlen(toWrite)," %30s |", "FIFO do Balcao Criado");
			break;
		case CLIASKSSERVICE:
			sprintf(toWrite + strlen(toWrite)," %30s |", "Cliente pede Atendimento");
			break;
		case BALCAOSTARTSSERVICE:
			sprintf(toWrite + strlen(toWrite)," %30s |", "Balcao começa Atendimento");
			break;
		case BALCAOSTOPSSERVICE:
			sprintf(toWrite + strlen(toWrite)," %30s |", "Balcao termina Atendimento");
			break;
		case CLIENTENDSERVICE:
			sprintf(toWrite + strlen(toWrite)," %30s |", "Cliente termina Atendimento");
			break;
		case CLOSEBALCAO:
			sprintf(toWrite + strlen(toWrite)," %30s |", "Balcao é fechado");
			break;
		case BALCAOSTOPSSERVING:
			sprintf(toWrite + strlen(toWrite)," %30s |", "Balcao pára Atendimentos");
			break;
		case STORECLOSE:
			sprintf(toWrite + strlen(toWrite)," %30s |", "Loja fecha");
			break;
	}
	char channelUsedTemp[DEFAULT_STRING_SIZE];
	strcpy(channelUsedTemp,channelUsed);
	if(channelUsedTemp[0] == '/')
		sscanf(channelUsedTemp,"/tmp/%s",channelUsedTemp);
	sprintf(toWrite + strlen(toWrite)," %-10s\n",channelUsedTemp);

	char SHM[DEFAULT_STRING_SIZE];

	sprintf(SHM,"%s.log",shmemName);
	FILE* ptr;
	if(event == SHMEMINIT){
		ptr = fopen(SHM,"w");
		fwrite("quando                  | quem   | balcao | o_que                            | canal_criado/usado  \n",1,DEFAULT_STRING_SIZE/2,ptr);
		fwrite("---------------------------------------------------------------------------------------------------\n",1,DEFAULT_STRING_SIZE/2,ptr);
	}
	else ptr = fopen(SHM,"a");

	fwrite(toWrite,1,strlen(toWrite),ptr);

	fclose(ptr);

};



void writeSHM(int* pt){
	FILE * ptr;
	char toWrite[DEFAULT_STRING_SIZE];
	ptr = fopen("SHM.txt","w");
	sprintf(toWrite,"Tempo de Abertura da Loja -> %i\n",pt[OPENINGTIME]);
	fwrite(toWrite,1,strlen(toWrite),ptr);

	int numberOfBalcoes = pt[NUMOFBALCOES];
	sprintf(toWrite,"Numero de Balcoes Abertos -> %i\n",numberOfBalcoes);
	fwrite(toWrite,1,strlen(toWrite),ptr);

	sprintf(toWrite,"Balcao |     Abertura    |   Nome   |          Num_clientes         | Tempo_medio\n");
	fwrite(toWrite,1,strlen(toWrite),ptr);

	sprintf(toWrite," #     | Tempo | Duracao |   FIFO   | em_atendimento | ja_atendidos | atendimento\n");
	fwrite(toWrite,1,strlen(toWrite),ptr);

	int i = 1;

	float somaClientes = 0;
	float somaTempo = 0;

	while(i <= numberOfBalcoes){
		if(pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 5] == 0)
			sprintf(toWrite,"%-7d|%-7d|%-9d|fb_%-7d|%-16d|%-16d|%-12d\n",i,pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 1],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 2],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 3],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 4],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 5],0);
		else sprintf(toWrite,"%-7d|%-7d|%-9d|fb_%-7d|%-16d|%-16d|%.6f\n",i,pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 1],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 2],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 3],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 4],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 5],(float)((float)pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 6]/(float)pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 5]));
		somaClientes += pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 5];
		somaTempo += pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 6];
		fwrite(toWrite,1,strlen(toWrite),ptr);
		i++;
	}
	if(somaClientes == 0)
	sprintf(toWrite,"\nTempo Medio Global : 0");
	else sprintf(toWrite,"\nTempo Medio Global : %.6f",somaTempo/somaClientes);
	fwrite(toWrite,1,strlen(toWrite),ptr);
	fclose(ptr);
};



#endif // LOGGER_H_
