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

#define DEFAULT_STRING_SIZE 200

//Shared Memory index

#define OPENINGTIME 0
#define NUMOFBALCOES 1
#define NUMOFOPENBALCOES 2
#define NUMOFACTIVEBALCOES 3
#define BALCAODEFINING 4
#define NUMOFBALCAOVARIABLES 7

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

	while(i <= numberOfBalcoes){
		sprintf(toWrite,"%-7d|%-7d|%-9d|fb_%-7d|%-16d|%-16d|%-12d\n",i,pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 1],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 2],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 3],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 4],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 5],pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(i-1) + 6]);
		fwrite(toWrite,1,strlen(toWrite),ptr);
		i++;
	}
	fclose(ptr);
};

#endif // LOGGER_H_
