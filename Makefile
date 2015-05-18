all : balcao.c ger_cl.c
	gcc balcao.c -o balcao -D_REENTRANT -lpthread -Wall
	gcc ger_cl.c -o ger_cl -D_REENTRANT -lpthread -Wall
