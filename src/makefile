CC= gcc
CCFLAGS= -g -o 
ENDFLAGS= -lcrypto -lssl	

all: client server main

main:
	$(CC) $(CCFLAGS) main main.c sock352.h sock352lib.c $(ENDFLAGS)

server:
	$(CC) $(CCFLAGS) server server.c sock352.h sock352lib.c $(ENDFLAGS)

client:
	$(CC) $(CCFLAGS) client client.c sock352.h sock352lib.c $(ENDFLAGS)
 	
clean:
	rm client server main
