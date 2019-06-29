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

#include "Errors.h"
#include "AuthenticationProvider.h"
#include "libs/mylib.h"
#include "libs/mydb.h"

/**
 * Struktura zawierające informacje o połączeniu i requeście.
 */
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
void appendErrorMessage(FILE * responseFile, int errorCode);
void appendErrorHttpCode(FILE * responseFile, int errorCode);
void appendToQuery(char * query, char * newQuery);

int respondToIndex(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToGetLogin(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToPostLogin(ConnectionInfo connectionInfo);
int respondToPostLogout(ConnectionInfo connectionInfo);
int respondToGetRegister(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToPostRegister(ConnectionInfo connectionInfo);
int respondToGetAddGroup(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToPostAddGroup(ConnectionInfo connectionInfo);
int respondToGetStudents(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToPostAddToGroup(ConnectionInfo connectionInfo);
int respondToGetAddTest(ConnectionInfo connectionInfo, int messageType, char * message);
int respondToPostAddTest(ConnectionInfo connectionInfo);

int respondToGetTests(ConnectionInfo connectionInfo, int messageType, char *message);
int respondToGetTestsByLecturer(FILE *responseFile, ConnectionInfo connectionInfo, int messageType, char *message, Session session);
int respondToPostStartTest(ConnectionInfo connectionInfo);
int respondToGetTestsByStudent(FILE *responseFile, ConnectionInfo connectionInfo, int messageType, char *message, Session session);
int respondToPostTest(ConnectionInfo connectionInfo);
int respondToPostSendTest(ConnectionInfo connectionInfo);
int respondToPostCheckTest(ConnectionInfo connectionInfo);

void handleError(ConnectionInfo connectionInfo, int errorCode);

#endif //EGZAMIN_REQUESTPROCESSOR_H
