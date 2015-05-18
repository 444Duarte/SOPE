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


#define DEFAULT_STRING_SIZE 200
#define PROJ_ID 'a'
#define MEMPATH "/tmp"
#define FIMDEATENDIMENTO "fim_atendimento"
#define FIMDEATENDIMENTOSIZE 15

//Shared Memory index

#define OPENINGTIME 0
#define NUMOFBALCOES 1
#define NUMOFOPENBALCOES 2
#define NUMOFACTIVEBALCOES 3
#define NUMOFCLIENTSSORTING 4
#define BALCAODEFINING 5
#define NUMOFBALCAOVARIABLES 7


void *chooseBalcao(void *arg){
	void *ret;
	int *pt = arg;
	int balcao, minClientes = INT_MAX, numBalcoes = pt[NUMOFBALCOES];
	while(numBalcoes > 0)
	{
		if(pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numBalcoes-1) + 2] != 1)
			if(minClientes > pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numBalcoes-1) + 4])
			{
				minClientes = pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numBalcoes-1) + 4];
				balcao = numBalcoes;
			}
	numBalcoes--;
	}
	ret = malloc(sizeof(int));
	*(int *)ret = balcao; 
	return ret;
}

int main(int argc, char *argv[])
{
	//argv[1] = nome da memória partilhada
	//argv[2] = numero de clientes a gerar

	/*confirmação da existencia de memoria partilhada*/
	char memPath[DEFAULT_STRING_SIZE];
	
	sprintf(memPath,"%s/%s",MEMPATH,argv[0]);

	key_t key = ftok(memPath,PROJ_ID);

	int shmid = shmget(key,0,IPC_EXCL);


	if(shmid == -1){
		printf("Não há memória partilhada aberta!\n");
		exit(0);
	}

	int *pt = shmat(shmid, 0, 0);

	if(pt[NUMOFOPENBALCOES] == 0){
		printf("Não há balcoes abertos!\n");
		exit(0);
	}

	int numberOfCli;

	sscanf(argv[2],"%i",&numberOfCli);

	pid_t pid;

	while(numberOfCli > 0){

		pid = fork();

		if(pid < 0){
			printf("Erro ao executar o fork de criação de Clientes\n");
			return 1;
		}
		else if(pid == 0){
			char privateFIFOPathname[DEFAULT_STRING_SIZE], balcaoFIFO[DEFAULT_STRING_SIZE], balcaoAnswer[DEFAULT_STRING_SIZE];
			int currentPID = getpid();
			sprintf(privateFIFOPathname,"/tmp/fc_%i",currentPID);
			if(mkfifo(privateFIFOPathname,0660) < 0){
				printf("Erro ao criar o fifo privado do Cliente\n");
				return 2;
			}
			fprintf(stderr,"1\n");
			
			pthread_t tid;
			void *retval; //valor de retorno da escolha do balcao
			pthread_create(&tid,NULL,chooseBalcao,pt);
			pthread_join(tid,&retval);
			int chosenBalcao = *(int *)retval;
			free (retval);// free the malloc in chooseBalcao

			pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(chosenBalcao-1) + 4]++;
			sprintf(balcaoFIFO,"/tmp/fb_%i",pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(chosenBalcao-1) + 3]);
			int fdBalcao = open(balcaoFIFO,O_WRONLY);
			fprintf(stderr,"2\n");
			write(fdBalcao,privateFIFOPathname,strlen(privateFIFOPathname)+1);
			close(fdBalcao);//not sure if its doing anything
			fprintf(stderr,"3\n");
			int fd = open(privateFIFOPathname,O_RDONLY);
			fprintf(stderr,"4\n");
			read(fd,balcaoAnswer,FIMDEATENDIMENTOSIZE+1);
			fprintf(stderr,"5 -> %s\n",balcaoAnswer);
			if(strcmp(balcaoAnswer,FIMDEATENDIMENTO) == 0)
				fprintf(stderr,"NICE BRO\n");
			fprintf(stderr,"6\n");
			unlink(privateFIFOPathname);// temporario
			return 0; // temporario
		}
		else{
			numberOfCli--;
		}
	}
	return 0;
}
