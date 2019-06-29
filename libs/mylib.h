#ifndef EGZAMIN_MYLIB_H
#define EGZAMIN_MYLIB_H

#include <bits/types/FILE.h>

#define MAX_HOSTNAME_LENGTH 80
#define SESSION_ID_LENGTH 15
#define TCP "tcp"
#define UDP "udp"

void runAsDaemon(int port);
int createTcpSocket();
void bindSocketWithPort(int serverFd, int port);
void startListening(int serverFd, int maxBacklog);
void bindAndListen(int serverFd, int port, int maxBacklog);
void connectWithServer(int connectionFd, char * address, int port);
int createSocketAndConnect(char * address, int port);
void getIp(char *hostname, char *ip);
void getHostname(char * address, char * hostname);
void readIpAddress(char * address, char * result);
int isIpAddress(char * address);
int readPort(char * given, char * protocol);
int acceptConnection(int serverSocket);
int processRequestData(FILE *requestHeaders, char *sessionId, char *requestBody);
void removeLF(char *text);
int findSessionIdInCookie(char * Cookie, char * sessionId);
void trim(char *text);
void removeCRLF(char * text);
int decodeUrlEncoded(char *encoded, char *decoded);
void createTimestamp(char * timestamp, int offsetInMinutes);

#endif