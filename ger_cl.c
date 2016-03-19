#include "logger.h"

/*This function chooses the bets balcao to join based on the number of clients in queue.
The chosen balcao will be the one that has the fewer clients on queue.
*/

void *chooseBalcao(void *arg){
	void *ret;
	int *pt = (int *)arg;
	int balcao = 0, minClientes = INT_MAX, numBalcoes = pt[NUMOFBALCOES], currentBalcao = 1;
	while(currentBalcao <= numBalcoes)
	{
		if(pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(currentBalcao-1) + 2] == -1)
			if(minClientes > pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(currentBalcao-1) + 4])
			{

				minClientes = pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(currentBalcao-1) + 4];
				balcao = currentBalcao;
			}

			currentBalcao++;
		}
		ret = malloc(sizeof(int));
		*(int *)ret = balcao; 
		return ret;
	}

	int main(int argc, char *argv[])
	{
	//argv[1] = name of the shared memory
	//argv[2] = number of clients

	/*confirmação da existencia de memoria partilhada*/
		char memPath[DEFAULT_STRING_SIZE];

		sprintf(memPath,"%s",argv[1]);

		int shmfd = shm_open(memPath,O_RDWR,0600); 

		if (argc != 3){
			fprintf( stderr, "Usage: %s <memoriaPartilhada> <numdeClientes>\n", argv[0]);
			exit(1);
		}

		int numberOfCli;

		sscanf(argv[2],"%i",&numberOfCli);

		if(numberOfCli <= 0)
		{
			fprintf(stderr, "Numero de Clientes Invalido\n");
			exit(2);
		}


		if(shmfd == -1){
			printf("Não há memória partilhada aberta!\n");
			exit(3);
		}

		if (ftruncate(shmfd,SHM_SIZE) < 0)
		{
			fprintf(stderr, "Erro ao executar truncate da SHM -> %s\n", strerror(errno));
			exit(4);
		};

		int *pt = (int *) mmap(0,SHM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
		if(pt == MAP_FAILED)
		{
			fprintf(stderr, "Erro ao executar o mapeamento da SHM -> %s\n", strerror(errno));
			exit(5);
		} 

		if(pt[NUMOFOPENBALCOES] == 0){
			printf("Não há balcoes abertos!\n");
			exit(6);
		};

		char semName[DEFAULT_STRING_SIZE];

		sprintf(semName,"/%s",memPath);

		sem_t *sem = sem_open(semName,0,0600,1);
		if(sem == SEM_FAILED)
		{
			perror("WRITER failure in sem_open()");
			exit(4);
		}

		pid_t pid;

/*Add number of clients to process
Is used to stop the balcao from closing while its being chosen*/

		sem_wait(sem);

		pt[NUMBEROFCLIENTS] = pt[NUMBEROFCLIENTS] + numberOfCli;

		sem_post(sem);


//cycle to generate the multiple clients
		while(numberOfCli > 0){

			pid = fork();

			if(pid < 0){
				printf("Erro ao executar o fork de criação de Clientes\n");
				exit(7);
			}
			else if(pid == 0){

				/*Create client FIFO*/
				char privateFIFOPathname[DEFAULT_STRING_SIZE], balcaoFIFO[DEFAULT_STRING_SIZE], balcaoAnswer[DEFAULT_STRING_SIZE];
				int currentPID = getpid();
				sprintf(privateFIFOPathname,"/tmp/fc_%d",currentPID);
				if(mkfifo(privateFIFOPathname,0660) < 0){
					printf("Erro ao criar o fifo privado do Cliente\n");
					exit(8);
				}

				

				pthread_t tid;
			void *retval; //return value of the chosen balcao
			sem_wait(sem);//using this semaphore to acess the shm, if not used the chosen balcao might be wrong
			pthread_create(&tid,NULL,chooseBalcao,pt);//choose what balcao to join
			pthread_join(tid,&retval);
			int chosenBalcao = *(int *)retval;
			free (retval);// free the malloc in chooseBalcao
			pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(chosenBalcao-1) + 4]++;
			sprintf(balcaoFIFO,"/tmp/fb_%d",pt[BALCAODEFINING + NUMOFBALCAOVARIABLES*(chosenBalcao-1) + 3]);//getting the chosen balcao FIFO ID
			int fdBalcao = open(balcaoFIFO,O_WRONLY);
			write(fdBalcao,privateFIFOPathname,strlen(privateFIFOPathname)+1);//writing to the chosen balcao the client FIFO ID
			close(fdBalcao);
			writeLOG(memPath,time(NULL),CLIENT,chosenBalcao,CLIASKSSERVICE,privateFIFOPathname);
			sem_post(sem);
			int fd = open(privateFIFOPathname,O_RDONLY);
			read(fd,balcaoAnswer,FIMDEATENDIMENTOSIZE+1);
			if(strcmp(balcaoAnswer,FIMDEATENDIMENTO) == 0)
				writeLOG(memPath,time(NULL),CLIENT,chosenBalcao,CLIENTENDSERVICE,privateFIFOPathname);
			unlink(privateFIFOPathname);
			sem_close(sem);
			return 0;
		}
		else{
			numberOfCli--;
		}
	}
	exit(0);
}