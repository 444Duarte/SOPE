
#include "logger.h"

/*function given in the slides
altered to work with this project
*/
int readline(int fd, char *str)
{
	char* ptr = str;
	int n;
	do
	{
		while((n = read(fd,str,1)) == 0);
			//fprintf(stderr, "->%s\n",str);
	}
	while (n>0 && *str++ != '\0');
	return strlen(ptr);
}
/*
	Client Serving Thread
	arg arg is the pathname* of the clientFIFO
*/
	void *clientServing(void * arg){

		//calculate the waiting Time
		struct atendimentoData *atendimento = (struct atendimentoData *) arg;
		int waitingTime = atendimento->argStruct->clientOnLine;
		waitingTime++;
		if(waitingTime > 10)
			waitingTime = 10;

		atendimento->argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(atendimento->argStruct->numberBalcao-1) + 6] += waitingTime;
		atendimento->argStruct->clientOnLine++;

		writeLOG(atendimento->argStruct->shmemName,time(NULL),BALCAO,atendimento->argStruct->numberBalcao,BALCAOSTARTSSERVICE,atendimento->clientFIFOID);
		sleep(waitingTime);

		//semaphore is used to sync the input on the shm
		sem_wait(atendimento->argStruct->sem);

		int fdCliente = open(atendimento->clientFIFOID,O_WRONLY);
		if(fdCliente<0)
			fprintf(stderr, "Erro ao Executar Open ao Fifo do Cliente -> %s\n", strerror(errno));
		write(fdCliente,FIMDEATENDIMENTO,FIMDEATENDIMENTOSIZE+1);
		close(fdCliente);
		atendimento->argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(atendimento->argStruct->numberBalcao-1) + 5]++;
		atendimento->argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(atendimento->argStruct->numberBalcao-1) + 4]--;
		writeLOG(atendimento->argStruct->shmemName,time(NULL),BALCAO,atendimento->argStruct->numberBalcao,BALCAOSTOPSSERVICE,atendimento->clientFIFOID);
		sem_post(atendimento->argStruct->sem);
		free(atendimento);
		return 0;
	}

	void balcaoManagement(struct balcaoData *arg){

		char privateFIFOPathname[DEFAULT_STRING_SIZE], clientFIFOID[DEFAULT_STRING_SIZE];
		sprintf(privateFIFOPathname,"/tmp/fb_%i",arg->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(arg->numberBalcao-1) + 3]);
		
		int fd = open(privateFIFOPathname,O_RDONLY);

		if(fd < 0){
			fprintf(stderr,"Erro ao abrir o FIFO do Balcao -> %s\n",strerror(errno));
		}
		while(readline(fd,clientFIFOID) > 0){

			struct atendimentoData *atendimento = malloc(sizeof(struct atendimentoData));
			atendimento->argStruct = arg;

			arg->pt[NUMBEROFCLIENTS]--;


			
			strcpy(atendimento->clientFIFOID, clientFIFOID);
			
			pthread_t tid;
			pthread_create(&tid,NULL,clientServing,atendimento);
		}
		fprintf(stderr,"Saiu do readline");// This is never going to run, since the while is broken by the signal
		return;	
	}


	void *cycle_function(void *arg){

		struct balcaoData *argStruct;

		argStruct = (struct balcaoData *) arg;

		time_t openingTime = time(NULL);

		int shmfd = shm_open(argStruct->shmemName,O_RDWR,0600); 

		char semName[DEFAULT_STRING_SIZE];

		sprintf(semName,"/%s",argStruct->shmemName);

		if(shmfd == -1){
		/*Create shm if its the first balcao*/
			shmfd = shm_open(argStruct->shmemName,O_CREAT|O_RDWR,0600);

			if (ftruncate(shmfd,SHM_SIZE) < 0)
			{
				fprintf(stderr, "Erro ao executar truncate da SHM -> %s\n", strerror(errno));
				exit(3);
			};

		/*create pointer to shared memory*/
			argStruct->pt = (int *) mmap(0,SHM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
			if(argStruct->pt == MAP_FAILED)
			{
				fprintf(stderr, "Erro ao executar o mapeamento da SHM -> %s\n", strerror(errno));
				exit(4);
			} 

			argStruct->sem = sem_open(semName,O_CREAT,0600,1);
			if(argStruct->sem == SEM_FAILED)
			{
				perror("WRITER failure in sem_open()");
				exit(5);
			} 

			//initiate the global variables of the shm
			writeLOG(argStruct->shmemName,openingTime,BALCAO,1,SHMEMINIT,"-");
			argStruct->pt[OPENINGTIME] = openingTime;
			argStruct->pt[NUMOFBALCOES] = 0;
			argStruct->pt[NUMOFOPENBALCOES] = 0;
			argStruct->pt[NUMOFACTIVEBALCOES] = 0;
			argStruct->pt[NUMBEROFCLIENTS] = 0;
		}
		else {
			argStruct->pt = (int *) mmap(0,SHM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0); /*create pointer to shared memory*/
			argStruct->sem = sem_open(semName,0,0600,1);
			if(argStruct->sem == SEM_FAILED)
			{
				perror("WRITER failure in sem_open()");
				exit(6);
			}
		}


	//Update of some variables
		argStruct->pt[NUMOFBALCOES] = argStruct->pt[NUMOFBALCOES] + 1;
		argStruct->numberBalcao = argStruct->pt[NUMOFBALCOES];
		argStruct->pt[NUMOFOPENBALCOES]++;
		argStruct->pt[NUMOFACTIVEBALCOES]++;
		fprintf(stderr,"Open Balcoes = %i\n",argStruct->pt[NUMOFOPENBALCOES]);
		fprintf(stderr,"Active Balcoes = %i\n",argStruct->pt[NUMOFACTIVEBALCOES]);

	/* creating balcao FIFO*/
		char privateFIFOPathname[DEFAULT_STRING_SIZE], privateFIFOID[DEFAULT_STRING_SIZE];
		int currentPID = getpid();
		sprintf(privateFIFOPathname,"/tmp/fb_%i",currentPID);
		sprintf(privateFIFOID,"fb_%i",currentPID);
		if(mkfifo(privateFIFOPathname,0660) < 0){
			printf("Erro ao criar o fifo do Balcao\n");
			exit(7);
		}
		writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,BALCAOFIFOCREATION,privateFIFOID);


	//create variables for this balcao
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1)] = argStruct->pt[NUMOFBALCOES];
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 1] = openingTime;
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 2] = -1; // tempo de abertura
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 3] = currentPID;
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 4] = 0; // clientes em espera
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 5] = 0; // clientes atendidos
	argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 6] = 0; // tempo total de atendimento

	/*main thread calling*/
	pid_t pid;
	pid = fork();

	if(pid == 0)
	{
		balcaoManagement(argStruct);
		exit(0);
	}
	else if(pid > 0)
	{
	/*Sleeping the inserted time
	Blocks the process*/
		sleep(argStruct->timeIsOpen);

		writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,CLOSEBALCAO,"-");

	/*closing the balcao to the customers, but balcao still working if needed*/
		argStruct->pt[NUMOFOPENBALCOES]--;
		fprintf(stderr,"Open Balcoes = %i\n",argStruct->pt[NUMOFOPENBALCOES]);
		while(argStruct->pt[NUMBEROFCLIENTS] != 0);
		argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 2] = argStruct->timeIsOpen;

	/*Cycle that checks if the balcao still has clients in line.
	Blocks the process again
	*/
	
	while(argStruct->pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(argStruct->numberBalcao-1) + 4] != 0);
	/*When there is no more clients a signal is send to stop the blocked FIFO*/
	kill(pid,SIGKILL);

	argStruct->pt[NUMOFACTIVEBALCOES]--;
	fprintf(stderr,"Active Balcoes = %i\n",argStruct->pt[NUMOFACTIVEBALCOES]);
	unlink(privateFIFOPathname);

	writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,BALCAOSTOPSSERVING,"-");

	/*Delete the shared memory if its the last balcao*/
	if(argStruct->pt[NUMOFACTIVEBALCOES] == 0){
		writeLOG(argStruct->shmemName,time(NULL),BALCAO,argStruct->numberBalcao,STORECLOSE,"-");
		writeSHM(argStruct->pt);

		sem_close(argStruct->sem);
		sem_unlink(semName); 

		if (munmap(argStruct->pt,SHM_SIZE) < 0)
		{
			fprintf(stderr, "Erro ao executar o desmapeamento da SHM -> %s\n", strerror(errno));
			exit(8);
		} 
		if (shm_unlink(argStruct->shmemName) < 0)
		{
			fprintf(stderr, "Erro ao executar o unlink da SHM -> %s\n", strerror(errno));
			exit(9);
		} 
	}else{
		if (munmap(argStruct->pt,SHM_SIZE) < 0)
		{
			fprintf(stderr, "Erro ao executar o desmapeamento da SHM -> %s\n", strerror(errno));
			exit(10);
		}
	}
	free((void *)argStruct->pt[BALCAODEFINING]);
	free(argStruct);
	exit(0);
}
else{
	fprintf(stderr,"Erro a criar o Fork -> %s\n",strerror(errno));
	exit(11);
}
exit(0);
}
/*
	Main function.
	It is used to creat the cycle_function thread.
*/
	int main(int argc, char *argv[])
	{
	//argv[1] = name of the shm
	//argv[2] = time the balcao will be open
	/*
		check if shared memory exists
	*/

		if (argc != 3){
			fprintf( stderr, "Usage: %s <memoriaPartilhada> <tempoAbertura>\n", argv[0]);
			exit(1);
		}

		struct balcaoData *argStruct = malloc(sizeof(struct balcaoData));
		argStruct->timeIsOpen = atoi(argv[2]);
		argStruct->shmemName = argv[1];
		argStruct->clientOnLine = 0;

		if(argStruct->timeIsOpen <= 0)
		{
			fprintf(stderr, "Erro na passagem do tempo de abertura\n");
			exit(1);
		}
		pid_t pid;
		pthread_t tid;
		if((pid = fork()) == 0){
			pthread_create(&tid,NULL,cycle_function,argStruct);
		}
		else if(pid < 0){
			fprintf(stderr, "Erro ao Fork -> %s\n", strerror(errno));
			exit(2);
		}
		pthread_exit(NULL);
	}