ReadMe - Simulação de serviço de atendimento ao público - T3G04

Exemplo de utilização: 

	1) ./bin/balcao shm 30
	2) ./bin/ger_cl shm 300
	3) ./bin/balcao shm 30
	4) ./bin/ger_cl shm 400
	5) ./bin/balcao shm 30

Os ficheiros enviados são o resultante da execução destas instruções, pela ordem apresentada.

Ao analisar o trabalho deparamo-nos com o problema de sincronizar a Shared Memory para tanto os Clientes como para os Balcoes.
Usamos 3 variáveis para resolver este problema:

NUMBEROFCLIENTS -> Numero de clientes que há para gerar em ger_cl mas que ainda não foi lançada a sua thread de atendimento.

NUMOFOPENBALCOES -> Permite saber quantos Balcoes estão abertos e prontos a receber novos clientes. O decremento desta variavel é bloqueada
enquanto a variavel NUMBEROFCLIENTS for diferente de 0.

NUMOFACTIVEBALCOES -> Numero de Balcoes que ainda estão em funcionamento mas não necessáriamente abertos para receber novos clientes. Enquanto houver ACTIVEBALCOES a loja não é encerrada.
