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
#define PROJ_ID 'a'
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

//global variables
int timeIsOpen;
int numberBalcao;
int *pt;

/*function given in the slides*/
int readline(int fd, char *str)
{
	int n;
	do
	{
		while((n = read(fd,str,1)) == 0);
	}
	while (n>0 && *str++ != '\0');
	return strlen(str);
}
/*
	Thread de Atendimento dos Clientes
	arg é o pathname do FIFO do cliente
*/
	void *clienteAtendimento(void * arg){
		int waitingTime = pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 4] + 1;
		if(waitingTime > 10)
			waitingTime = 10;
		sleep(waitingTime);
		int fdCliente = open(arg,O_WRONLY);
		write(fdCliente,FIMDEATENDIMENTO,FIMDEATENDIMENTOSIZE+1);
		close(fdCliente);
		pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 5]++;
		pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 6] = 123; /*estas contas vão ser bonitas vão -.-*/
		pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 4]--;
		return 0;
	}

	void balcaoManagement(){
		char privateFIFOPathname[DEFAULT_STRING_SIZE], clientFIFOID[DEFAULT_STRING_SIZE];
		sprintf(privateFIFOPathname,"/tmp/fb_%i",pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 3]);

		int fd = open(privateFIFOPathname,O_RDONLY);

		fprintf(stderr,"Passou o bloqueio!!\n");

		if(fd < 0){
			fprintf(stderr,"Erro ao abrir o FIFO do Balcao -> %s\n",strerror(errno));
		}

		while(readline(fd,clientFIFOID) > 0){
			fprintf(stderr, "Entrou no readline ->%s\n",clientFIFOID);
			pthread_t tid;
			pthread_create(&tid,NULL,clienteAtendimento,clientFIFOID);
			pthread_join(tid,NULL); // apagado quando o mutexe for feito
		}
		fprintf(stderr,"Saiu do readline");
		return;
	}


	void *cycle_function(void *arg){


		time_t openingTime = time(NULL);
		time_t current_time = openingTime;

		key_t key = ftok((char *)arg,PROJ_ID);
		int shmid = shmget(key,1024,0);


		if(shmid == -1){
		/*no caso de não ter sido criado a memória partilhada*/
		shmid = shmget(key,1024,IPC_CREAT|SHM_R|SHM_W);//tamanho aleatorio

		/*create pointer to shared memory*/
		pt = (int *)shmat(shmid,0,0);
		/*defining the shared memory variables*/
		pt[OPENINGTIME] = openingTime;
		pt[NUMOFBALCOES] = 0;
		pt[NUMOFOPENBALCOES] = 0;
		pt[NUMOFACTIVEBALCOES] = 0;
		pt[NUMOFCLIENTSSORTING] = 0;
	}
	else pt = (int *)shmat(shmid,0,0); /*create pointer to shared memory*/
	
	//Update of some variables
	pt[NUMOFBALCOES] = pt[NUMOFBALCOES] + 1;
	numberBalcao = pt[NUMOFBALCOES];
	pt[NUMOFOPENBALCOES]++;
	pt[NUMOFACTIVEBALCOES]++;
	fprintf(stderr,"Open Balcoes = %i\n",pt[NUMOFOPENBALCOES]);
	fprintf(stderr,"Active Balcoes = %i\n",pt[NUMOFACTIVEBALCOES]);

	/* creating balcao FIFO*/
	char privateFIFOPathname[DEFAULT_STRING_SIZE];
	int currentPID = getpid();
	sprintf(privateFIFOPathname,"/tmp/fb_%i",currentPID);
	if(mkfifo(privateFIFOPathname,0660) < 0){
		printf("Erro ao criar o fifo do Balcao\n");
		return 0;
	}

	//create variables for this balcao
	pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1)] = pt[NUMOFBALCOES];
	pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 1] = openingTime;
	pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 2] = -1; // tempo de abertura
	pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 3] = currentPID;
	pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 4] = 0; // clientes em espera
	pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 5] = 0; // clientes atendidos
	pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 6] = 0; // tempo medio de atendimento

	/*main thread calling*/
	pid_t pid;
	pid = fork();

	if(pid == 0)
	{
		balcaoManagement();
		return 0;
	}
	else if(pid > 0)
	{
	/*cycling through time to close balcao
	Bloqueia este processo*/
		while(difftime(current_time,openingTime) < timeIsOpen)
			current_time = time(NULL);

		/*closing the balcao to the customers, but balcao still working if needed*/
		pt[NUMOFOPENBALCOES]--;
		fprintf(stderr,"Open Balcoes = %i\n",pt[NUMOFOPENBALCOES]);

		pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 2] = timeIsOpen;

	/*Ciclo para confirmar se existem clientes em fila ou não
	Bloqueia novamente este pocesso
	*/
	while(pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(numberBalcao-1) + 4] != 0);
	/*Quando não houver enviar sinal para parar a espera bloqueante*/
	kill(pid,SIGKILL);

	int status;
	waitpid(pid,&status,0);

	pt[NUMOFACTIVEBALCOES]--;
	fprintf(stderr,"Active Balcoes = %i\n",pt[NUMOFACTIVEBALCOES]);
	unlink(privateFIFOPathname);

	/*apagar a memoria partilhado no caso de ser o ultimo balcao*/
	if(pt[NUMOFACTIVEBALCOES] == 0)
		shmctl(shmid,IPC_RMID,NULL);//temporario
	return 0;
}
else{
	fprintf(stderr,"Erro a criar o Fork -> %s\n",strerror(errno));
}
return 0;
}
/*
	Main function.
	It is used to creat the cycle_function thread.
*/
	int main(int argc, char *argv[])
	{
	//argv[1] = nome da memória partilhada
	//argv[2] = tempo de abertura do balcao
	/*
		confirmação da existencia de memoria partilhada
	*/
		timeIsOpen = atoi(argv[2]);

		pthread_t tid;

		if(fork() == 0){
			pthread_create(&tid,NULL,cycle_function,argv[1]);
		}
	//shmctl(shmid,IPC_RMID,NULL);
		pthread_exit(NULL);
	}