CC = gcc

all: main.c
    service mysqld start
	gcc -Wall main.c libs/mylib.c libs/mydb.c libs/mybase64.c RequestProcessor.c AuthenticationProvider.c $(mysql_config --cflags) -o egzamin $(mysql_config --libs)