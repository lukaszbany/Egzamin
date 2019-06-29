CC = gcc

all: main.c
	gcc -Wall main.c libs/mylib.c libs/mydb.c libs/mybase64.c RequestProcessor.c AuthenticationProvider.c $(shell mysql_config --cflags) -o egzamin $(shell mysql_config --libs)