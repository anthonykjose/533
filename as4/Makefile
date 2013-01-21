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


all: akjose_arp akjose_tour
#	get_hw_addrs.o prhwaddrs.o ${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

#app_functions.o: app_functions.c
#	${CC} ${CFLAGS} -c app_functions.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

arp_api.o: arp_api.c
	${CC} ${CFLAGS} -c arp_api.c

akjose_arp: arp.o get_hw_addrs.o arp_api.o
	${CC} ${FLAGS} -o akjose_arp arp.o get_hw_addrs.o arp_api.o ${LIBS}
arp.o: arp.c
	${CC} ${CFLAGS} -c arp.c

akjose_tour: tour_app.o get_hw_addrs.o arp_api.o
	${CC} ${FLAGS} -o akjose_tour tour_app.o get_hw_addrs.o arp_api.o ${LIBS}
tour_app.o: tour_app.c
	${CC} ${CFLAGS} -c tour_app.c

	
clean:
	rm akjose_arp arp.o get_hw_addrs.o arp_api.o akjose_tour

