cc = gcc
ftpserver : server_main.o  server_sub.o 
	$(cc)  server_main.o server_sub.o   -o ftpserver
server_main.o : server_main.c server.h
	$(cc) -c server_main.c
server_sub.o : server_sub.c server.h
	$(cc) -c server_sub.c
clean :
	rm ftpserver *.o 