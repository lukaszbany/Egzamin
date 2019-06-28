#ifndef EGZAMIN_REQUESTPROCESSOR_H
#define EGZAMIN_REQUESTPROCESSOR_H

#include <bits/types/FILE.h>
#include <stdio.h>
#include <mysql/mysql.h>
#include <zconf.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "ErrorHandling.h"
#include "AuthenticationProvider.h"
#include "libs/mylib.h"
#include "libs/mydb.h"

typedef struct ConnectionInfo {
    int connectionFd;
    char * request;
    char * headers;
    char * sessionId;
    char * requestBody;
} ConnectionInfo;

void process(int connectionFd);
struct ConnectionInfo createConnectionInfo(int connectionFd, char *request, char *headers, char * sessionId, char * requestBody);
int resolveRequest(ConnectionInfo connectionInfo);
void generateNewSessionId(char *sessionId);
int isFileRequest(char * request, char * headers);
void appendToHeaders(char * headers, char * newHeader);
void getRequestMethod(char *request, char * method);
void readArgumentFromRequest(char *request, char * argument);
int resolveGetFileRequest(ConnectionInfo connectionInfo);
int sendFile(int connectionFd, char * filename, char * headers);
int isMappedRequest(char * request);
int resolveMappedRequest(ConnectionInfo connectionInfo);
int appendPart(FILE *responseFile, char *part);
void appendMenu(FILE *responseFile, Session session);
void appendUserData(FILE *responseFile, Session session);
int getPostParameterValue(char * requestBody, char * parameter, char * value, int mandatory);
void appendMessage(FILE * responseFile, int messageType, char * message);

int respondToIndex(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToGetLogin(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToPostLogin(ConnectionInfo connectionInfo);

#endif //EGZAMIN_REQUESTPROCESSOR_H
