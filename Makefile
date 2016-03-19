all : balcao.c ger_cl.c
	gcc balcao.c -o bin/balcao -D_REENTRANT -lpthread -lrt -Wall
	gcc ger_cl.c -o bin/ger_cl -D_REENTRANT -lpthread -lrt -Wall
