#include <stdio.h>
#include <mysql/mysql.h>
#include <zconf.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#include "libs/mylib.h"
#include "RequestProcessor.h"

/**
 * Domyślny port serwera.
 */
#define DEFAULT_PORT 9876

/**
 * Liczba wywołanych na początku programu procesów, które obsługują żądania.
 */
#define MAX_CLIENTS 20

/**
 * Maksymalna ilość uczekujących połączeń.
 */
#define MAX_BACKLOG 3

int getPort(int argc, char *const *argv);


/**
 * Metoda startująca serwer. Jako parametr można podać port
 * (jeśli nie zostanie podany, użyty zostanie domyślny 9876)
 *
 * @param argc liczba argumentów
 * @param argv tablica argumentów
 * @return 0 jeśli zostanie zakończony poprawnie
 */
int main(int argc, char *argv[])
{
    int serverSocket, connectionFd, activeConnections = 0;

    setlocale(LC_ALL, "polish");

    int port = getPort(argc, argv);
    serverSocket = createTcpSocket(port);
    bindAndListen(serverSocket, port, MAX_BACKLOG);
    while(1){
        if (activeConnections < MAX_CLIENTS) {
            int pid = fork();
            if (pid == -1) {
                printf("Error creating process");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                connectionFd = acceptConnection(serverSocket);
                process(connectionFd);

                close(connectionFd);
            } else if (pid > 0) {
                activeConnections++;
            }
        } else {
            sleep(1);
        }
    }
}

/**
 * Pobiera port z argumentu lub ustawia domyślny
 * @param argc liczba argumentów
 * @param argv tablica argumentów
 * @return numer portu
 */
int getPort(int argc, char *const *argv) {
    int port;
    if (argc < 2) {
        port = DEFAULT_PORT;
    } else {
        port = readPort(argv[1], TCP);
    }
    return port;
}