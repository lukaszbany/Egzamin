#ifndef EGZAMIN_MYDB_H
#define EGZAMIN_MYDB_H
#include <mysql/mysql.h>
#include <stdio.h>
#include <sys/syslog.h>
#include <string.h>

typedef struct Session {
    char * sessionId;
    int userId;
    int loggedIn;
    char * login;
    char * firstName;
    char * lastName;
    char * role;
    char * group;
} Session;

typedef struct User {
    int id;
    char * login;
    char * password;
    char * firstName;
    char * lastName;
    char * role;
    int groupId;
} User;

int databaseError(MYSQL *connection);
int runQuery(MYSQL *connection, char *query);
MYSQL_RES *getResult(MYSQL *connection);
MYSQL *connectToDatabase();
int saveSession(Session session);
int saveSessionData(char * sessionId, int userId, int loggedIn);
int getSession(char * sessionId, Session * session);
int getUserByLogin(char * sessionId, User * user);

#endif //EGZAMIN_MYDB_H
