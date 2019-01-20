CC=gcc
CFLAGS=-std=c11 -Wall -g -lreadline 

all: clean mpsh

mpsh: mpsh.c completion.c completion.h intern_command.c intern_command.h
	$(CC) mpsh.c intern_command.c completion.c  $(CFLAGS) -o mpsh  

clean:
	$(RM) -rf mpsh
