include ./Make.defines
#include /home/users/cse533/Stevens/unpv13e/Make.defines
# This is a sample Makefile which compiles source files named:
# - tcpechotimeserv.c
# - tcpechotimecliv.c
# - time_cli.c
# - echo_cli.c
# and creating executables: "server", "client", "time_cli"
# and "echo_cli", respectively.
#
# It uses various standard libraries, and the copy of Stevens'
# library "libunp.a" in ~cse533/Stevens/unpv13e_solaris2.10 .
#
# It also picks up the thread-safe version of "readline.c"
# from Stevens' directory "threads" and uses it when building
# the executable "server".
#
# It is set up, for illustrative purposes, to enable you to use
# the Stevens code in the ~cse533/Stevens/unpv13e_solaris2.10/lib
# subdirectory (where, for example, the file "unp.h" is located)
# without your needing to maintain your own, local copies of that
# code, and without your needing to include such code in the
# submissions of your assignments.
#
# Modify it as needed, and include it with your submission.

CC = gcc

LIBS = /home/users/cse533/Stevens/unpv13e/libunp.a -lpthread\

FLAGS = -w -g -O2

CFLAGS = ${FLAGS} -I/home/users/cse533/Stevens/unpv13e/lib


all: client odr server  \
#	get_hw_addrs.o prhwaddrs.o ${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

app_functions.o: app_functions.c
	${CC} ${CFLAGS} -c app_functions.c

client: client.o app_functions.o
	${CC} ${FLAGS} -o client client.o app_functions.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c

server: server.o app_functions.o
	${CC} ${FLAGS} -o server server.o app_functions.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

odr: odr.o get_hw_addrs.o prhwaddrs.o
	${CC} ${FLAGS} -o odr odr.o get_hw_addrs.o prhwaddrs.o app_functions.o ${LIBS}
odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c
	
clean:
	rm client server odr

