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
#define INSERT_USER_TEMPLATE "INSERT INTO user (login, password, first_name, last_name, role) VALUES ('%s', '%s', '%s', '%s', '%s')"


/**
 * Uruchamia zapytanie do bazy.
 *
 * @param connection aktywne połączenie z bazą
 * @param query zapytanie SQL
 * @return 0 w razie sukcesu lub 500 w razie błędu bazy
 */
int runQuery(MYSQL *connection, char *query) {
    if (mysql_query(connection, query)) {
        return databaseError(connection);
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
 * @param sessionId numer sesji do zapisania
 * @return 0 w razie sukcesu lub 500 w razie błędu bazy
 */
int saveSession(Session session) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        mysql_close(connection);
        return INTERNAL_SERVER_ERROR;
    }

    char userId[5];
    if (session.userId == 0) {
        strcpy(userId, "NULL");
    } else {
        sprintf(userId, "%d", session.userId);
    }

    char getSessionQuery[MAX_QUERY];
    sprintf(getSessionQuery, GET_SESSION_TEMPLATE, session.sessionId);
    errorCode = runQuery(connection, getSessionQuery);
    if (errorCode) {
        return databaseError(connection);
    }

    MYSQL_RES *result = getResult(connection);

    char insertSessionQuery[BUFSIZ];
    sprintf(insertSessionQuery, INSERT_SESSION_TEMPLATE, session.sessionId, userId, session.loggedIn, userId, session.loggedIn);
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
 * Zapisuje daną sesję do bazy, jeśli nie była zapisana
 * wcześniej. Jeśli była, modyfikuje wpis.
 *
 * @param sessionId numer sesji - parametr wymagany
 * @param userId powiązany użytkownik - niewymagany
 * @param loggedIn czy użytkownik jest zalogowany - niewymagane
 * @return 0 w razie sukcesu lub 500 w razie błędu bazy
 */
int saveSessionData(char * sessionId, int userId, int loggedIn) {
    Session session;
    session.sessionId = sessionId;
    session.userId = userId;
    session.loggedIn = loggedIn;

    return saveSession(session);
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
        mysql_close(connection);
        return INTERNAL_SERVER_ERROR;
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
        session->sessionId = row[0] ? row[0] : NULL;
        session->userId = row[1] ? atoi(row[1]) : 0;
        session->loggedIn = row[2] ? atoi(row[2]) : 0;

        if (session->loggedIn) {
            session->login = row[4] ? row[4] : 0;
            session->firstName = row[6] ? row[6] : 0;
            session->lastName = row[7] ? row[7] : 0;
            session->role = row[8] ? row[8] : 0;
            session->group = row[10] ? row[10] : 0;
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
        mysql_close(connection);
        return INTERNAL_SERVER_ERROR;
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
            user->login = row[1] ? row[1] : NULL;
            user->password = row[2] ? row[2] : NULL;
            user->firstName = row[3] ? row[3] : NULL;
            user->lastName = row[4] ? row[4] : NULL;
            user->role = row[5] ? row[5] : NULL;
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

int addUser(char * login, char * password, char * firstName, char * lastName, char * role) {
    int errorCode = 0;
    MYSQL *connection = connectToDatabase();
    if (!connection) {
        mysql_close(connection);
        return INTERNAL_SERVER_ERROR;
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
            0, NULL, 0) == NULL) {
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
    fprintf(stderr, "Error connecting to database: %s", mysql_error(connection));
    syslog(LOG_ERR, "Error connecting to database: %s", mysql_error(connection));
    mysql_close(connection);
    return INTERNAL_SERVER_ERROR;
}