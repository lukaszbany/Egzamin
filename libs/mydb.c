#include "mydb.h"

#define INTERNAL_SERVER_ERROR 500
#define NOT_FOUND 404

#define DB_SERVER "localhost"
#define DB_DATABASE "egzamin"
#define DB_LOGIN "egzamin"
#define DB_PASSWORD "6}H46aeEr&#G"

#define MAX_QUERY 256

#define GET_SESSION_TEMPLATE "SELECT s.*, u.*, g.name FROM session s LEFT JOIN user u ON s.user_id = u.id LEFT JOIN student_group g ON u.group_id = g.id WHERE session_id = '%s'"
#define INSERT_SESSION_TEMPLATE "INSERT INTO session (session_id, user_id, logged_in) VALUES ('%s', %s, %d) ON DUPLICATE KEY UPDATE user_id = %s, logged_in = %d"

#define GET_USER_BY_LOGIN_TEMPLATE "SELECT * FROM user WHERE login = '%s'"
#define GET_STUDENTS_TEMPLATE "SELECT u.*, g.name FROM user u LEFT JOIN student_group g ON u.group_id = g.id WHERE u.role = 'STUDENT'"
#define GET_STUDENTS_BY_GROUP_TEMPLATE "SELECT u.*, g.name FROM user u LEFT JOIN student_group g ON u.group_id = g.id WHERE u.role = 'STUDENT' AND u.group_id = %d"
#define INSERT_USER_TEMPLATE "INSERT INTO user (login, password, first_name, last_name, role) VALUES ('%s', '%s', '%s', '%s', '%s')"
#define UPDATE_USER_SET_GROUP_TEMPLATE "UPDATE user SET group_id = '%d' WHERE id = '%d'"

#define GET_GROUPS_TEMPLATE "SELECT * FROM student_group"
#define GET_GROUP_BY_NAME_TEMPLATE "SELECT * FROM student_group WHERE name = '%s'"
#define INSERT_GROUP_TEMPLATE "INSERT INTO student_group (name) VALUES ('%s')"

#define INSERT_TEST "INSERT INTO test (author_id, subject) VALUES (%d, '%s')"
#define GET_TESTS_TEMPLATE "SELECT * FROM test WHERE author_id = %d "
#define GET_QUESTIONS_TEMPLATE "SELECT * FROM question WHERE test_id = %d"

#define GET_WAITING_TESTS_BY_TEST_AND_GROUP_TEMPLATE "SELECT * FROM waiting_test WHERE test_id = %d AND group_id = %d"
#define GET_WAITING_TEST_BY_ID_TEMPLATE "SELECT w.*, t.subject FROM waiting_test w JOIN test t on t.id = w.test_id WHERE w.id = %d"
#define GET_WAITING_TEST_BY_ID_AND_NOT_ENDED_TEMPLATE "SELECT w.*, t.subject FROM waiting_test w JOIN test t on t.id = w.test_id WHERE w.id = %d AND w.end_time > CURRENT_TIMESTAMP()"
#define GET_WAITING_TESTS_BY_AUTHOR_AND_NOT_ENDED_TEMPLATE "SELECT w.*, t.subject, t.author_id, g.name FROM waiting_test w JOIN test t on t.id = w.test_id JOIN student_group g ON g.id = w.group_id WHERE t.author_id = %d AND w.end_time < CURRENT_TIMESTAMP() AND w.id NOT IN (SELECT waiting_test_id FROM result)"
#define GET_WAITING_TESTS_BY_STUDENT_AND_GROUP_AND_NOT_ENDED_AND_NOT_CHECKED_TEMPLATE "SELECT w.*, t.subject FROM waiting_test w JOIN test t on t.id = w.test_id WHERE w.group_id = %d AND w.end_time > CURRENT_TIMESTAMP() AND w.id NOT IN (SELECT waiting_test_id FROM test_answer WHERE student_id = %d)"
#define INSERT_WAITING_TEST_TEMPLATE "INSERT INTO waiting_test (test_id, group_id, end_time) VALUES (%d, %d, '%s')"
#define GET_WAITING_TESTS_BY_STUDENT_AND_ENDED_AND_CHECKED_TEMPLATE "SELECT w.*, t.subject, r.result FROM waiting_test w JOIN test t on t.id = w.test_id JOIN result r ON r.waiting_test_id = w.id WHERE r.student_id = %d AND r.result IS NOT NULL"


#define GET_ANSWERS_BY_WAITING_TEST_AND_STUDENT_TEMPLATE "SELECT * FROM test_answer WHERE waiting_test_id = %d AND student_id = %d"
#define GET_ANSWERS_BY_WAITING_TEST_TEMPLATE "SELECT * FROM test_answer WHERE waiting_test_id = %d"
#define INSERT_ANSWERS_TEMPLATE "INSERT INTO test_answer (waiting_test_id, student_id, answers) VALUES (%d, %d, '%s')"

/**
 * Uruchamia zapytanie do bazy.
 *
 * @param connection aktywne połączenie z bazą
 * @param query zapytanie SQL
 * @return 0 w razie sukcesu lub -1 w razie błędu bazy
 */
int runQuery(MYSQL *connection, char *query) {
    if (mysql_query(connection, query)) {
        return -1;
    }

    return 0;
}

/**
 * Pobiera wynik ostatniego zapytania do bazy.
 *
 * @param connection aktywne połączenie z bazą
 * @return 0 w razie sukcesu lub 500 w razie błędu bazy
 */
MYSQL_RES *getResult(MYSQL *connection) {
    MYSQL_RES *result = mysql_store_result(connection);

    if (result == NULL) {
        databaseError(connection);
    }

    return result;
}

/**
 * Zapisuje daną sesję do bazy, jeśli nie była zapisana
 * wcześniej. Jeśli była, modyfikuje wpis.
 *
 * @param sessionId numer sesji - parametr wymagany
 * @param userId powiązany użytkownik - niewymagany
 * @param loggedIn czy użytkownik jest zalogowany - niewymagane
 * @return 0 w razie sukcesu lub 500 w razie błędu bazy
 */
int saveSession(char * sessionId, int userId, int loggedIn) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char userIdToSave[5];
    if (userId == 0) {
        strcpy(userIdToSave, "NULL");
    } else {
        sprintf(userIdToSave, "%d", userId);
    }

    char getSessionQuery[MAX_QUERY];
    sprintf(getSessionQuery, GET_SESSION_TEMPLATE, sessionId);
    errorCode = runQuery(connection, getSessionQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);

    char insertSessionQuery[BUFSIZ];
    sprintf(insertSessionQuery, INSERT_SESSION_TEMPLATE, sessionId, userIdToSave, loggedIn, userIdToSave, loggedIn);
    errorCode = runQuery(connection, insertSessionQuery);
    if (errorCode) {
        mysql_free_result(result);
        return databaseError(connection);
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera dane o sesji z bazy.
 *
 * @param sessionId numer sesji do pobrania
 * @param session dane sesji, które zostaną uzupełnione
 * @return 0 w razie sukcesu, 500 w razie błędu bazy
 */
int getSession(char * sessionId, Session * session) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getSessionQuery[BUFSIZ];
    sprintf(getSessionQuery, GET_SESSION_TEMPLATE, sessionId);
    errorCode = runQuery(connection, getSessionQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (result->row_count > 0) {
        strcpy(session->sessionId, row[0]);
        session->userId = row[1] ? atoi(row[1]) : 0;
        session->loggedIn = row[2] ? atoi(row[2]) : 0;

        if (session->loggedIn) {
            if(row[4]) strcpy(session->login, row[4]);
            if(row[6]) strcpy(session->firstName, row[6]);
            if(row[7]) strcpy(session->lastName, row[7]);
            if(row[8]) strcpy(session->role, row[8]);
            session->groupId = row[9] ? atoi(row[9]) : 0;
            if(row[10]) strcpy(session->group, row[10]);
        }
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera dane o użytkowniku z bazy po loginie
 * @param login do wyszukania użytkownika
 * @param user dane o użytkowniku, które zostaną uzupełnione
 * @return 0 w razie sukcesu, -1 w razie nie znalezienia
 * użytkownika o podanym loginie, 500 w razie błędu bazy
 */
int getUserByLogin(char * login, User * user) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getUserQuery[BUFSIZ];
    sprintf(getUserQuery, GET_USER_BY_LOGIN_TEMPLATE, login);
    errorCode = runQuery(connection, getUserQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (result->row_count > 0) {
        if (user != NULL) {
            user->id = row[0] ? atoi(row[0]) : 0;
            if(row[1]) strcpy(user->login, row[1]);
            if(row[2]) strcpy(user->password, row[2]);
            if(row[3]) strcpy(user->firstName, row[3]);
            if(row[4]) strcpy(user->lastName, row[4]);
            if(row[5]) strcpy(user->role, row[5]);
            user->groupId = row[6] ? atoi(row[6]) : 0;
        }

        mysql_free_result(result);
        mysql_close(connection);
        return 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return -1;
}

/**
 * Pobiera listę wszystkich studentów.
 * @param user wynikowa tablica studentów
 * @param count liczba znalezionych wpisów
 * @param session sesja
 * @return
 */
int getStudents(User user[], int * count, Session * session) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getStudentsQuery[BUFSIZ];
    sprintf(getStudentsQuery, GET_STUDENTS_TEMPLATE);
    errorCode = runQuery(connection, getStudentsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row;;
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        row = mysql_fetch_row(result);

        user[i].id = row[0] ? atoi(row[0]) : 0;
        if(row[3]) strcpy(user[i].firstName, row[3]);
        if(row[4]) strcpy(user[i].lastName, row[4]);
        if(row[5]) strcpy(user[i].role, row[5]);
        user[i].groupId = row[6] ? atoi(row[6]) : 0;
        if(row[7]) strcpy(user[i].groupName, row[7]);
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera tablicę id studentów należących do podanej grupy.
 * @param ids tablica id do zapełnienia
 * @param count liczba znalezionych wpisów
 * @param groupId id grupy
 * @return 0 w razie sukcesu, 500 w razie błędu bazy
 */
int getStudentsIdsByGroup(int ids[], int * count, int groupId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getStudentsQuery[BUFSIZ];
    sprintf(getStudentsQuery, GET_STUDENTS_BY_GROUP_TEMPLATE, groupId);
    errorCode = runQuery(connection, getStudentsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row;;
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        row = mysql_fetch_row(result);

        ids[i] = row[0] ? atoi(row[0]) : 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera testy dla utworzone przez zalogowanego użytkownika
 * @param tests tablica testów do zapełnienia
 * @param count liczba znalezionych wpisów
 * @param session aktualna sesja
 * @return
 */
int getTestsByAuthor(Test tests[], int * count, Session * session) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getTestsQuery[BUFSIZ];
    sprintf(getTestsQuery, GET_TESTS_TEMPLATE, session->userId);
    errorCode = runQuery(connection, getTestsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row;
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        row = mysql_fetch_row(result);

        tests[i].id = row[0] ? atoi(row[0]) : 0;
        if(row[2]) strcpy(tests[i].subject, row[2]);
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Dodaje oczekujący test dla podanej grup z podanym czasem koncowym.
 * @param testId numer testu
 * @param groupId numer grupy
 * @param endTime Timestamp z czasem zakończenia
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int addWaitingTest(int testId, int groupId, char * endTime) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char insertWaitingTestQuery[MAX_QUERY];
    sprintf(insertWaitingTestQuery, INSERT_WAITING_TEST_TEMPLATE, testId, groupId, endTime);
    errorCode = runQuery(connection, insertWaitingTestQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    mysql_close(connection);
    return 0;
}

/**
 * Pobiera oczekujący test dla podanego id testu i grupy.
 * @param testId id testu
 * @param groupId id grupy
 * @return 0 w razie znalezienia, -1 w razie nie znalezienia,
 * 500 w razie błędu bazy
 */
int getWaitingTestByTestAndGroup(int testId, int groupId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getWaitingTestsQuery[BUFSIZ];
    sprintf(getWaitingTestsQuery, GET_WAITING_TESTS_BY_TEST_AND_GROUP_TEMPLATE, testId, groupId);
    errorCode = runQuery(connection, getWaitingTestsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);

    if (result->row_count > 0) {
        mysql_free_result(result);
        mysql_close(connection);
        return 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return -1;
}

/**
 * Pobiera oczekujące i nie zakończone testy dla studenta i grupy, które nie zostały jeszcze sprawdzone.
 *
 * @param waitingTests tablica testów do zapelnienia
 * @param count liczba znalezionych wpisów
 * @param groupId
 * @param studentId
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int getWaitingTestsByStudentAndGroupAndNotEnded(WaitingTest *waitingTests, int *count, int groupId, int studentId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getWaitingTestsQuery[BUFSIZ];
    sprintf(getWaitingTestsQuery, GET_WAITING_TESTS_BY_STUDENT_AND_GROUP_AND_NOT_ENDED_AND_NOT_CHECKED_TEMPLATE, groupId, studentId);
    errorCode = runQuery(connection, getWaitingTestsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        MYSQL_ROW row = mysql_fetch_row(result);

        waitingTests[i].id = row[0] ? atoi(row[0]) : 0;
        waitingTests[i].testId = row[1] ? atoi(row[1]) : 0;
        waitingTests[i].groupId = row[2] ? atoi(row[2]) : 0;
        if(row[4]) strcpy(waitingTests[i].subject, row[4]);
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera testy utworzone przez użytkownika, których czas minął
 * @param waitingTests
 * @param count liczba znalezionych wpisów
 * @param authorId id autora
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int getWaitingTestByAuthorAndEnded(WaitingTest *waitingTests, int *count, int authorId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getWaitingTestsQuery[BUFSIZ];
    sprintf(getWaitingTestsQuery, GET_WAITING_TESTS_BY_AUTHOR_AND_NOT_ENDED_TEMPLATE, authorId);
    errorCode = runQuery(connection, getWaitingTestsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        MYSQL_ROW row = mysql_fetch_row(result);

        waitingTests[i].id = row[0] ? atoi(row[0]) : 0;
        waitingTests[i].testId = row[1] ? atoi(row[1]) : 0;
        waitingTests[i].groupId = row[2] ? atoi(row[2]) : 0;
        if(row[4]) strcpy(waitingTests[i].subject, row[4]);
        if(row[6]) strcpy(waitingTests[i].group, row[6]);
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera sprawdzone testy studenta
 * @param waitingTests tablica testów do zapełnienia
 * @param count liczba znalezionych wpisów
 * @param studentId id studenta
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int getCheckedTestByStudent(WaitingTest waitingTests[], int *count, int studentId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getWaitingTestsQuery[BUFSIZ];
    sprintf(getWaitingTestsQuery, GET_WAITING_TESTS_BY_STUDENT_AND_ENDED_AND_CHECKED_TEMPLATE, studentId);
    errorCode = runQuery(connection, getWaitingTestsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        MYSQL_ROW row = mysql_fetch_row(result);

        waitingTests[i].id = row[0] ? atoi(row[0]) : 0;
        waitingTests[i].testId = row[1] ? atoi(row[1]) : 0;
        waitingTests[i].groupId = row[2] ? atoi(row[2]) : 0;
        if(row[4]) strcpy(waitingTests[i].subject, row[4]);
        waitingTests[i].result = row[5] ? atoi(row[5]) : 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera oczekując test po id, jeśli się nie zakończył
 * @param waitingTest wynikowy test
 * @param waitingTestId id oczekującego testu do znalezienia
 * @return 0 w razie znalezienia, -1 w razie nie znalezienia,
 * 500 w razie błędu bazy */
int getWaitingTestByIdAndNotEnded(WaitingTest * waitingTest, int waitingTestId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getWaitingTestQuery[BUFSIZ];
    sprintf(getWaitingTestQuery, GET_WAITING_TEST_BY_ID_AND_NOT_ENDED_TEMPLATE, waitingTestId);
    errorCode = runQuery(connection, getWaitingTestQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (result->row_count > 0) {
        waitingTest->id = row[0] ? atoi(row[0]) : 0;
        waitingTest->testId = row[1] ? atoi(row[1]) : 0;
        waitingTest->groupId = row[2] ? atoi(row[2]) : 0;
        if(row[4]) strcpy(waitingTest->subject, row[4]);

        mysql_free_result(result);
        mysql_close(connection);
        return 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return -1;
}

/**
 * Pobiera oczekujący test po jego id
 * @param waitingTest wynikowy test
 * @param waitingTestId id oczekującego testu do znalezienia
 * @return 0 w razie znalezienia, -1 w razie nie znalezienia,
 * 500 w razie błędu bazy */
int getWaitingTestById(WaitingTest * waitingTest, int waitingTestId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getWaitingTestQuery[BUFSIZ];
    sprintf(getWaitingTestQuery, GET_WAITING_TEST_BY_ID_TEMPLATE, waitingTestId);
    errorCode = runQuery(connection, getWaitingTestQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (result->row_count > 0) {
        waitingTest->id = row[0] ? atoi(row[0]) : 0;
        waitingTest->testId = row[1] ? atoi(row[1]) : 0;
        waitingTest->groupId = row[2] ? atoi(row[2]) : 0;
        if(row[4]) strcpy(waitingTest->subject, row[4]);

        mysql_free_result(result);
        mysql_close(connection);
        return 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return -1;
}

/**
 * Pobiera pytania, które są częścią podanego testu.
 * @param questions tablica pytań do zapełnienia
 * @param count liczba znalezionych wpisów
 * @param testId id testu
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int getQuestionsByTestId(Question questions[], int * count, int testId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getQuestionsQuery[BUFSIZ];
    sprintf(getQuestionsQuery, GET_QUESTIONS_TEMPLATE, testId);
    errorCode = runQuery(connection, getQuestionsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        MYSQL_ROW row = mysql_fetch_row(result);

        questions[i].id = row[0] ? atoi(row[0]) : 0;
        questions[i].testId = row[1] ? atoi(row[1]) : 0;
        if(row[2]) strcpy(questions[i].question, row[2]);
        if(row[3]) strcpy(questions[i].answer1, row[3]);
        if(row[4]) strcpy(questions[i].answer2, row[4]);
        if(row[5]) strcpy(questions[i].answer3, row[5]);
        if(row[6]) strcpy(questions[i].answer4, row[6]);
        if(row[7]) questions[i].correct = row[7][0];
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Dodaje użytkownika do bazy
 * @param login
 * @param password
 * @param firstName imię
 * @param lastName nazwisko
 * @param role rola w systemie
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int addUser(char * login, char * password, char * firstName, char * lastName, char * role) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char insertUserQuery[MAX_QUERY];
    sprintf(insertUserQuery, INSERT_USER_TEMPLATE, login, password, firstName, lastName, role);
    errorCode = runQuery(connection, insertUserQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    mysql_close(connection);
    return 0;
}

/**
 * Dodaje odpowiedzi studenta na test.
 * @param waitingTestId id oczekującego testu
 * @param student_id id studenta
 * @param answers odpowiedzi studenta w formie łancucha znaków
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int addAnswers(int waitingTestId, int student_id, char * answers) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char insertAnswersQuery[MAX_QUERY];
    sprintf(insertAnswersQuery, INSERT_ANSWERS_TEMPLATE, waitingTestId, student_id, answers);
    errorCode = runQuery(connection, insertAnswersQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    mysql_close(connection);
    return 0;
}

/**
 * Pobiera odpowiedzi studenta na podany oczekujący test.
 * @param answers wynikowe odpowiedzi
 * @param waitingTestId id oczekującego testu
 * @param studentId id studenta
 * @return 0 w razie znalezienia, -1 w razie nie znalezienia,
 * 500 w razie błędu bazy */
int getAnswersByWaitingTestIdAndStudentId(char * answers, int waitingTestId, int studentId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getAnswersQuery[BUFSIZ];
    sprintf(getAnswersQuery, GET_ANSWERS_BY_WAITING_TEST_AND_STUDENT_TEMPLATE, waitingTestId, studentId);
    errorCode = runQuery(connection, getAnswersQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (result->row_count > 0) {
        if (answers != NULL && row[3] != NULL) {
            strcpy(answers, row[3]);
        }

        mysql_free_result(result);
        mysql_close(connection);
        return 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return -1;
}

/**
 * Pobiera odpowiedzi studentów na podany oczekujacy test
 * @param answers tablica odpowiedzi do zapełnienia
 * @param count liczba znalezionych wpisów
 * @param waitingTestId id oczekującego testu
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int getAnswersByWaitingTestId(Answers answers[], int * count, int waitingTestId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getAnswersQuery[BUFSIZ];
    sprintf(getAnswersQuery, GET_ANSWERS_BY_WAITING_TEST_TEMPLATE, waitingTestId);
    errorCode = runQuery(connection, getAnswersQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);

    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        MYSQL_ROW row = mysql_fetch_row(result);

        answers[i].studentId = row[2] ? atoi(row[2]) : 0;
        if (row[3]) strcpy(answers[i].answers, row[3]);
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Uruchamia podane zapytanie do bazy
 * @param query treść zapytania
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int runPreparedQuery(char * query) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    errorCode = runQuery(connection, query);
    if (errorCode) {
        return databaseError(connection);
    }

    mysql_close(connection);
    return 0;
}

/**
 * Dodaje test do bazy
 * @param userId id autora
 * @param subject temat testu
 * @param testId id wstawionego właśnie testu
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int addTest(int userId, char * subject, int * testId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char insertUserQuery[MAX_QUERY];
    sprintf(insertUserQuery, INSERT_TEST, userId, subject);
    errorCode = runQuery(connection, insertUserQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    *testId = mysql_insert_id(connection);
    mysql_close(connection);
    return 0;
}

/**
 * Dodaje grupę do studenta.
 * @param userId id studenta
 * @param groupId id grupy
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int updateUserSetGroup(int userId, int groupId) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char updateUserQuery[MAX_QUERY];
    sprintf(updateUserQuery, UPDATE_USER_SET_GROUP_TEMPLATE, groupId, userId);
    errorCode = runQuery(connection, updateUserQuery);
    if (errorCode != 0) {
        return databaseError(connection);
    }

    mysql_close(connection);
    return 0;
}

/**
 * Pobiera grupy z bazy
 * @param groups tablica grup do zapełnienia
 * @param count liczba znalezionych wpisów
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int getGroups(Group groups[], int * count) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getGroupsQuery[BUFSIZ];
    sprintf(getGroupsQuery, GET_GROUPS_TEMPLATE);
    errorCode = runQuery(connection, getGroupsQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row;;
    *count = result->row_count;

    for (int i = 0; i < *count; i++) {
        row = mysql_fetch_row(result);

        groups[i].id = row[0] ? atoi(row[0]) : 0;
        if(row[1]) strcpy(groups[i].name, row[1]);
    }

    mysql_free_result(result);
    mysql_close(connection);
    return 0;
}

/**
 * Pobiera grupę po nazwie
 * @param name nazwa grupy do znalezienia
 * @param group znaleziona grupa
 * @return 0 w razie znalezienia, -1 w razie nie znalezienia,
 * 500 w razie błędu bazy */
int getGroupByName(char * name, Group * group) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char getGroupQuery[BUFSIZ];
    sprintf(getGroupQuery, GET_USER_BY_LOGIN_TEMPLATE, name);
    errorCode = runQuery(connection, getGroupQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (result->row_count > 0) {
        if (group != NULL) {
            group->id = row[0] ? atoi(row[0]) : 0;
            if(row[1]) strcpy(group->name, row[1]);
        }

        mysql_free_result(result);
        mysql_close(connection);
        return 0;
    }

    mysql_free_result(result);
    mysql_close(connection);
    return -1;
}

/**
 * Dodaje grupę do bazy
 * @param name nazwa grupy
 * @return 0 w razie sukcesu, 500 w razie błędu bazy.
 */
int addGroup(char * name) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        return databaseError(connection);
    }

    char insertGroupQuery[MAX_QUERY];
    sprintf(insertGroupQuery, INSERT_GROUP_TEMPLATE, name);
    errorCode = runQuery(connection, insertGroupQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    mysql_close(connection);
    return 0;
}

/**
 * Uruchamia połączenie z bazą (wymagane, żeby wykonywać operacje na bazie).
 *
 * @return wskaźnik do struktury MYSQL zawierającej informacje
 * o uzyskanym połączeniu lub NULL, jeśli połączenie się nie udało.
 */
MYSQL *connectToDatabase() {
    MYSQL *connection = mysql_init(NULL);
    if (connection == NULL) {
        databaseError(connection);
        return NULL;
    }

    if (mysql_real_connect(
            connection,
            DB_SERVER,
            DB_LOGIN,
            DB_PASSWORD,
            DB_DATABASE,
            0, NULL, CLIENT_MULTI_STATEMENTS) == NULL) {
        databaseError(connection);
        return NULL;
    }

    return connection;
}

/**
 * Loguje lub wypisuje na ekran komunikat błędu dotyczącego bazy,
 * zamyka połączenie i zwraca błąd INTERNAL_SERVER_ERROR.
 *
 * @param connection połączenie z bazą, którego dotyczy błąd
 * @return 500 (INTERNAL_SERVER_ERROR)
 */
int databaseError(MYSQL *connection) {
    fprintf(stderr, "Database error: %s\n", mysql_error(connection));
    syslog(LOG_ERR, "Database error: %s", mysql_error(connection));
    mysql_close(connection);
    return INTERNAL_SERVER_ERROR;
}