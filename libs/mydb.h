#ifndef EGZAMIN_MYDB_H
#define EGZAMIN_MYDB_H

#define FIELD_LENGTH 96
#define LONG_FIELD_LENGTH 256

#define INSERT_QUESTION "INSERT INTO question (test_id, question, answer1, answer2, answer3, answer4, correct_answer) VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s');\n"


#include <mysql/mysql.h>
#include <stdio.h>
#include <sys/syslog.h>
#include <string.h>

typedef struct Session {
    char sessionId[FIELD_LENGTH];
    int userId;
    int loggedIn;
    char login[FIELD_LENGTH];
    char firstName[FIELD_LENGTH];
    char lastName[FIELD_LENGTH];
    char role[FIELD_LENGTH];
    int groupId;
    char group[FIELD_LENGTH];
} Session;

typedef struct User {
    int id;
    char login[FIELD_LENGTH];
    char password[FIELD_LENGTH];
    char firstName[FIELD_LENGTH];
    char lastName[FIELD_LENGTH];
    char role[FIELD_LENGTH];
    int groupId;
    char groupName[FIELD_LENGTH];
} User;

typedef struct Group {
    int id;
    char name[FIELD_LENGTH];
} Group;

typedef struct Question {
    int id;
    int testId;
    char question[LONG_FIELD_LENGTH];
    char answer1[FIELD_LENGTH];
    char answer2[FIELD_LENGTH];
    char answer3[FIELD_LENGTH];
    char answer4[FIELD_LENGTH];
    char correct;
} Question;

typedef struct Test {
    int id;
    char subject[FIELD_LENGTH];
} Test;

typedef struct Answers {
    int studentId;
    char answers[12];
} Answers;

typedef struct WaitingTest {
    int id;
    int testId;
    int groupId;
    char group[FIELD_LENGTH];
    char subject[FIELD_LENGTH];
    int result;
} WaitingTest;

int databaseError(MYSQL *connection);
int runQuery(MYSQL *connection, char *query);
MYSQL_RES *getResult(MYSQL *connection);
MYSQL *connectToDatabase();
int runPreparedQuery(char * query);

int saveSession(char * sessionId, int userId, int loggedIn);
int getSession(char * sessionId, Session * session);

int getUserByLogin(char * sessionId, User * user);
int getStudents(User user[], int * count, Session * session);
int getStudentsIdsByGroup(int ids[], int * count, int groupId);
int addUser(char * login, char * password, char * firstName, char * lastName, char * role);
int updateUserSetGroup(int userId, int groupId);

int getGroups(Group groups[], int * count);
int getGroupByName(char * name, Group * group);
int addGroup(char * name);

int addTest(int userId, char * subject, int * testId);
int getTestsByAuthor(Test tests[], int * count, Session * session);

int addWaitingTest(int testId, int groupId, char * endTime);
int getWaitingTestByTestAndGroup(int testId, int groupId);
int getWaitingTestsByStudentAndGroupAndNotEnded(WaitingTest *waitingTests, int *count, int groupId, int studentId);
int getWaitingTestByIdAndNotEnded(WaitingTest * waitingTest, int waitingTestId);
int getWaitingTestByAuthorAndEnded(WaitingTest *waitingTests, int *count, int authorId);
int getWaitingTestById(WaitingTest * waitingTest, int waitingTestId);
int getAnswersByWaitingTestId(Answers answers[], int * count, int waitingTestId);
int getCheckedTestByStudent(WaitingTest waitingTests[], int *count, int studentId);

int getQuestionsByTestId(Question questions[], int * count, int testId);
int addAnswers(int waitingTestId, int student_id, char * answers);
int getAnswersByWaitingTestIdAndStudentId(char * answers, int waitingTestId, int studentId);


#endif //EGZAMIN_MYDB_H
