#include "RequestProcessor.h"

/**
 * Mini bufor (128 znaków)
 */
#define MINI_BUF 128

/**
 * Mały bufor (365 znaków)
 */
#define SMALL_BUF 256

/**
 * Maksymalna długość metody HTTP
 */
#define METHOD_LENGTH 8

/**
 * Maksymalna wielkość tablicy struktur bazy danych
 */
#define MAX_ARRAY 100

/**
 * Standardowa odpowiedź HTTP (OK).
 */
#define ANSWER_OK "HTTP/1.1 200 OK\r\n"

/**
 * Argument wymagany metody post
 */
#define MANDATORY 1
/**
 * Argument opcjonalny metody post
 */
#define OPTIONAL 0

/**
 * Brak wiadomości do użytkownika
 */
#define NO_MESSAGE 0
/**
 * Wiadomość do użytkownika informująca o sukcesie
 */
#define SUCCESS 1
/**
 * Wiadomość do użytkownika informująca o niepowodzeniu
 */
#define ALERT 2

/**
 * Zerowy offset przesunięcia minutowego czasu.
 */
#define NOW 0

/**
 * Znaki, z których może składać się sesja.
 */
const char sessionChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPRSTUVWXYZ";


/**
 * Przetwarza request na podanym gnieździe.
 * @param connectionFd numer gniazda
 */
void process(int connectionFd) {
    char request[SMALL_BUF] = "";
    char headers[BUFSIZ] = "";
    char sessionId[SESSION_ID_LENGTH] = "";
    char requestBody[BUFSIZ] = "";
    int errorCode = 0;

    FILE *requestFile = fdopen(connectionFd, "r");
    fgets(request, BUFSIZ, requestFile);
    if (!strstr(request, "HTTP/1.")) {
        printf("BAD REQUEST %s", request);
        fclose(requestFile);
        return;
    }

    printf("Process %d received request: %s", getpid(), request);

    int oldSessionFound = processRequestData(requestFile, sessionId, requestBody);

    if (oldSessionFound == -1) {
        generateNewSessionId(sessionId);
        errorCode = saveSession(sessionId, 0, 0);
        if (errorCode) {
            ConnectionInfo connectionInfo = {connectionFd, NULL, NULL, NULL, NULL};
            handleError(connectionInfo, errorCode);
            fclose(requestFile);
            return;
        }
        sprintf(headers, "Set-Cookie: SESSION_ID=%s", sessionId);
    }

    ConnectionInfo connectionInfo = createConnectionInfo(connectionFd, request, headers, sessionId, requestBody);

    errorCode = resolveRequest(connectionInfo);
    if (errorCode) {
        handleError(connectionInfo, errorCode);
        fclose(requestFile);
        return;
    }

    fclose(requestFile);
}

/**
 * Przetwarza request zgodnie informacjami ze struktury connectionInfo
 * @param connectionInfo informacje o requeście
 * @return zwraca 0 w razie sukcesu lub numer błędu w razie jego wystąpienia
 */
int resolveRequest(ConnectionInfo connectionInfo) {
    if (isFileRequest(connectionInfo.request, connectionInfo.headers)) {
        return resolveGetFileRequest(connectionInfo);
    } else if (isMappedRequest(connectionInfo.request)) {
        return resolveMappedRequest(connectionInfo);
    }

    return NOT_FOUND;
}

/**
 * Sprawdza, czy zapytanie dotyczy pliku i czy wysyłanie
 * podanego pliku jest dozwolone. Jeśli tak, to dodaje
 * odpowiedni nagłówek.
 *
 * @param request treść requestu
 * @param headers nagłówki
 * @return 1 jeśli zapytanie o plik, 0 jeśli nie
 */
int isFileRequest(char *request, char *headers) {
    char method[METHOD_LENGTH];
    getRequestMethod(request, method);
    if (!strcmp(request, "GET")) {
        return BAD_REQUEST;
    }

    if (strstr(request, ".css")) {
        appendToHeaders(headers, "Content-type: text/css");
        return 1;
    } else if (strstr(request, ".js")) {
        appendToHeaders(headers, "Content-type: text/javascript");
        return 1;
    } else if (strstr(request, ".png")) {
        appendToHeaders(headers, "Content-type: image/png");
        return 1;
    } else if (strstr(request, ".jpg")) {
        appendToHeaders(headers, "Content-type: image/jpeg");
        return 1;
    } else if (strstr(request, ".svg")) {
        appendToHeaders(headers, "Content-type: image/svg");
        return 1;
    } else if (strstr(request, ".ico")) {
        appendToHeaders(headers, "Content-type: image/x-icon");
        return 1;
    }

    return 0;
}

/**
 * Sprawdza czy podany request jest obsługiwany przez serwer.
 *
 * @param request treść requestu
 * @return 1 jeśli zapytanie o plik, 0 jeśli nie
 */
int isMappedRequest(char * request) {
    char method[METHOD_LENGTH];
    getRequestMethod(request, method);
    if (!strcmp(request, "GET")) {
        return NOT_FOUND;
    }

    char argument[SMALL_BUF];
    readArgumentFromRequest(request, argument);
    if (strcmp(argument, "/") == 0) {
        return 1;
    } else if (strcmp(argument, "/login") == 0) {
        return 1;
    } else if (strcmp(argument, "/register") == 0) {
        return 1;
    } else if (strcmp(argument, "/logout") == 0) {
        return 1;
    } else if (strcmp(argument, "/addGroup") == 0) {
        return 1;
    } else if (strcmp(argument, "/students") == 0) {
        return 1;
    } else if (strcmp(argument, "/addToGroup") == 0) {
        return 1;
    } else if (strcmp(argument, "/addTest") == 0) {
        return 1;
    } else if (strcmp(argument, "/tests") == 0) {
        return 1;
    } else if (strcmp(argument, "/startTest") == 0) {
        return 1;
    } else if (strcmp(argument, "/test") == 0) {
        return 1;
    } else if (strcmp(argument, "/sendTest") == 0) {
        return 1;
    } else if (strcmp(argument, "/checkTest") == 0) {
        return 1;
    }

    return 0;
}

/**
 * Przetwarza request, który jest obsługiwany przez serwer,
 * a nie jest requestem o plik.
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int resolveMappedRequest(ConnectionInfo connectionInfo) {
    char argument[SMALL_BUF] = "";
    char method[METHOD_LENGTH] = "";
    readArgumentFromRequest(connectionInfo.request, argument);
    getRequestMethod(connectionInfo.request, method);

    if (strcmp(argument, "/") == 0) {
        return respondToIndex(connectionInfo, NO_MESSAGE, NULL);
    } else if (strcmp(argument, "/login") == 0 && strcmp(method, "GET") == 0) {
        return respondToGetLogin(connectionInfo, NO_MESSAGE, NULL);
    } else if (strcmp(argument, "/login") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostLogin(connectionInfo);
    } else if (strcmp(argument, "/logout") == 0 && strcmp(method, "GET") == 0) {
        return respondToPostLogout(connectionInfo);
    } else if (strcmp(argument, "/register") == 0 && strcmp(method, "GET") == 0) {
        return respondToGetRegister(connectionInfo, NO_MESSAGE, NULL);
    } else if (strcmp(argument, "/register") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostRegister(connectionInfo);
    } else if (strcmp(argument, "/addGroup") == 0 && strcmp(method, "GET") == 0) {
        return respondToGetAddGroup(connectionInfo, NO_MESSAGE, NULL);
    } else if (strcmp(argument, "/addGroup") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostAddGroup(connectionInfo);
    } else if (strcmp(argument, "/students") == 0 && strcmp(method, "GET") == 0) {
        return respondToGetStudents(connectionInfo, NO_MESSAGE, NULL);
    } else if (strcmp(argument, "/addToGroup") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostAddToGroup(connectionInfo);
    } else if (strcmp(argument, "/addTest") == 0 && strcmp(method, "GET") == 0) {
        return respondToGetAddTest(connectionInfo, NO_MESSAGE, NULL);
    } else if (strcmp(argument, "/addTest") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostAddTest(connectionInfo);
    } else if (strcmp(argument, "/tests") == 0 && strcmp(method, "GET") == 0) {
        return respondToGetTests(connectionInfo, NO_MESSAGE, NULL);
    } else if (strcmp(argument, "/startTest") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostStartTest(connectionInfo);
    } else if (strcmp(argument, "/test") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostTest(connectionInfo);
    } else if (strcmp(argument, "/sendTest") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostSendTest(connectionInfo);
    } else if (strcmp(argument, "/checkTest") == 0 && strcmp(method, "POST") == 0) {
        return respondToPostCheckTest(connectionInfo);
    }

    return NOT_FOUND;
}

/**
 * Zwraca stronę główną serwisu
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToIndex(ConnectionInfo connectionInfo, int messageType, char * message) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "index-top");
    appendMessage(responseFile, messageType, message);
    appendPart(responseFile, "index-content");
    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Zwraca stronę logowania
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetLogin(ConnectionInfo connectionInfo, int messageType, char * message) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "login-top");
    if (session.loggedIn) appendMessage(responseFile, ALERT, "Jesteś już zalogowany!");
    if (messageType != NO_MESSAGE) appendMessage(responseFile, messageType, message);
    if (!session.loggedIn) appendPart(responseFile, "login-form");
    fprintf(responseFile, "</div>");

    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Zwraca stronę rejestracji
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetRegister(ConnectionInfo connectionInfo, int messageType, char * message) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "register-top");
    if (session.loggedIn) appendMessage(responseFile, ALERT, "Jesteś już zalogowany!");
    if (messageType != NO_MESSAGE) appendMessage(responseFile, messageType, message);
    if (!session.loggedIn) appendPart(responseFile, "register-form");
    fprintf(responseFile, "</div>");

    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Wysyła formularz rejestracyjny do serwera
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostRegister(ConnectionInfo connectionInfo) {
    int errors = 0;
    char login[MINI_BUF];
    char password[MINI_BUF];
    char firstName[MINI_BUF];
    char lastName[MINI_BUF];
    char role[MINI_BUF];
    errors += getPostParameterValue(connectionInfo.requestBody, "login", login, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "password", password, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "first-name", firstName, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "last-name", lastName, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "role", role, 1);
    if (errors != 0) {
        return respondToGetRegister(connectionInfo, ALERT, "Brak wymaganych pól!");
    }

    int errorCode = getUserByLogin(login, NULL);
    if (errorCode == 0) {
        return respondToGetRegister(connectionInfo, ALERT, "Taki użytkownik już istnieje!");
    }
    if (errorCode > 0) {
        return errorCode;
    }

    char encodedPassword[MINI_BUF];
    encodeChars(password, encodedPassword);

    errorCode = addUser(login, encodedPassword, firstName, lastName, role);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToIndex(connectionInfo, SUCCESS, "Zostałeś zarejestrowany. Teraz możesz się zalogować.");
}

/**
 * Zwraca stronę z dodawaniem nowej grupy
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetAddGroup(ConnectionInfo connectionInfo, int messageType, char * message) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn || strcmp(session.role, "ADMIN") != 0) {
        return UNAUTHORIZED;
    }

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "add-group-top");
    if (messageType != NO_MESSAGE) appendMessage(responseFile, messageType, message);
    appendPart(responseFile, "add-group-form");
    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Tworzy nową grupę
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostAddGroup(ConnectionInfo connectionInfo) {
    int errors = 0;
    char name[MINI_BUF];
    errors += getPostParameterValue(connectionInfo.requestBody, "name", name, MANDATORY);
    if (errors != 0) {
        return respondToGetAddGroup(connectionInfo, ALERT, "Brak nazwy grupy!");
    }

    int errorCode = getGroupByName(name, NULL);
    if (errorCode == 0) {
        return respondToGetAddGroup(connectionInfo, ALERT, "Taka grupa już istnieje!");
    }
    if (errorCode > 0) {
        return errorCode;
    }

    errorCode = addGroup(name);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToGetAddGroup(connectionInfo, SUCCESS, "Grupa została utworzona.");
}

/**
 * Dodaje studenta do grupy
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostAddToGroup(ConnectionInfo connectionInfo) {
    int errors = 0;
    char student[MINI_BUF];
    char group[MINI_BUF];
    errors += getPostParameterValue(connectionInfo.requestBody, "student", student, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "group", group, MANDATORY);
    if (errors != 0) {
        return respondToGetStudents(connectionInfo, ALERT, "Nie udało się dodać studenta do grupy!");
    }

    int errorCode = updateUserSetGroup(atoi(student), atoi(group));
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToGetStudents(connectionInfo, SUCCESS, "Dodano studenta do grupy");
}

/**
 * Udostępnia test grupie
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostStartTest(ConnectionInfo connectionInfo) {
    int errors = 0;
    char test[MINI_BUF];
    char group[MINI_BUF];
    char minutes[MINI_BUF];
    errors += getPostParameterValue(connectionInfo.requestBody, "test", test, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "group", group, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "time", minutes, MANDATORY);
    if (errors != 0) {
        return respondToGetTests(connectionInfo, ALERT, "Nie udało się udostępnić testu!");
    }

    int testId = atoi(test);
    int groupId = atoi(group);

    int errorCode = getWaitingTestByTestAndGroup(testId, groupId);
    if (errorCode == 0) {
        return respondToGetTests(connectionInfo, ALERT, "Test został już udostępniony tej grupie");
    } else if (errorCode > 0) {
        return errorCode;
    }

    char timestamp[MINI_BUF];
    createTimestamp(timestamp, atoi(minutes));

    errorCode = addWaitingTest(testId, groupId, timestamp);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToGetTests(connectionInfo, SUCCESS, "Test został udostępniony");
}

/**
 * Zwraca stronę z listą studentów
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetStudents(ConnectionInfo connectionInfo, int messageType, char * message) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    Session session = {0};
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn || strcmp(session.role, "ADMIN") != 0) {
        return UNAUTHORIZED;
    }

    int studentsCount = 0;
    User students[MAX_ARRAY] = {{0}};

    int errorCode = getStudents(students, &studentsCount, &session);
    if (errorCode != 0) {
        return errorCode;
    }

    int groupsCount = 0;
    Group groups[MAX_ARRAY] = {{0}};
    errorCode = getGroups(groups, &groupsCount);
    if (errorCode != 0) {
        return errorCode;
    }

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "students-top");
    if (messageType != NO_MESSAGE) appendMessage(responseFile, messageType, message);
    appendPart(responseFile, "students-table-top");

    for(int i = 0; i < studentsCount; i++) {

        if (students[i].groupId == 0) {
            fprintf(responseFile, "<tr><td>%d</td><td>%s</td><td>%s</td><td>",
                    i + 1, students[i].firstName, students[i].lastName);
            appendPart(responseFile, "students-table-group-top");
            fprintf(responseFile, "<input type=\"hidden\" value=\"%d\" name=\"student\" id=\"student\">", students[i].id);
            fprintf(responseFile, "<select class=\"form-control col-7\" id=\"group\" name=\"group\">");

            for(int j = 0; j < groupsCount; j++) {
                fprintf(responseFile, "<option value=\"%d\">%s</option>", groups[j].id, groups[j].name);
            }

            appendPart(responseFile, "students-table-group-bottom");
            fprintf(responseFile, "</td></tr>");
        } else {
            fprintf(responseFile, "<tr><td>%d</td><td>%s</td><td>%s</td><td>%s</td></tr>",
                    i + 1, students[i].firstName, students[i].lastName, students[i].groupName);
        }
    }

    appendPart(responseFile, "students-table-bottom");
    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Zwraca stronę z listą testów
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetTests(ConnectionInfo connectionInfo, int messageType, char *message) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    Session session = {0};
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn) {
        return UNAUTHORIZED;
    }

    if (strcmp(session.role, "EGZAMINATOR") == 0) {
        return respondToGetTestsByLecturer(responseFile, connectionInfo, messageType, message, session);
    } else if (strcmp(session.role, "STUDENT") == 0) {
        return respondToGetTestsByStudent(responseFile, connectionInfo, messageType, message, session);
    }

    fclose(responseFile);
    return UNAUTHORIZED;
}

/**
 * Zwraca stronę z listą testów dla egzaminatora
 * @param responseFile
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @param session
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetTestsByLecturer(
        FILE *responseFile, ConnectionInfo connectionInfo, int messageType, char *message, Session session) {

    int testCount = 0;
    Test tests[MAX_ARRAY] = {{0}};

    int errorCode = getTestsByAuthor(tests, &testCount, &session);
    if (errorCode != 0) {
        return errorCode;
    }

    int groupsCount = 0;
    Group groups[MAX_ARRAY] = {{0}};
    errorCode = getGroups(groups, &groupsCount);
    if (errorCode != 0) {
        return errorCode;
    }

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);

    // Utworzone testy:
    appendPart(responseFile, "created-tests-top");
    if (messageType != NO_MESSAGE) appendMessage(responseFile, messageType, message);
    appendPart(responseFile, "created-tests-table-top");

    for(int i = 0; i < testCount; i++) {
        fprintf(responseFile, "<tr><td>%d</td><td>%s</td><td>",
                tests[i].id, tests[i].subject);
        appendPart(responseFile, "created-tests-table-group-top");
        fprintf(responseFile, "<input type=\"hidden\" value=\"%d\" name=\"test\" id=\"test\">", tests[i].id);
        fprintf(responseFile, "<select class=\"form-control col-2\" id=\"group\" name=\"group\">");

        for(int j = 0; j < groupsCount; j++) {
            fprintf(responseFile, "<option value=\"%d\">%s</option>", groups[j].id, groups[j].name);
        }

        appendPart(responseFile, "created-tests-table-group-bottom");
        fprintf(responseFile, "</td></tr>");
    }

    appendPart(responseFile, "created-tests-table-bottom");

    //////////////////////
    // Zakończone testy:

    int count = 0;
    WaitingTest waitingTests[MAX_ARRAY] = {{0}};
    errorCode = getWaitingTestByAuthorAndEnded(waitingTests, &count, session.userId);
    if (errorCode != 0) {
        return errorCode;
    }

    appendPart(responseFile, "ended-tests-top");
    appendPart(responseFile, "ended-tests-table-top");

    for(int i = 0; i < count; i++) {
        fprintf(responseFile, "<tr><td>%d</td><td>%s</td><td>%s</td><td>",
                (i + 1), waitingTests[i].subject, waitingTests[i].group);
        appendPart(responseFile, "ended-tests-table-group-top");
        fprintf(responseFile, "<input type=\"hidden\" value=\"%d\" name=\"test\" id=\"test\">", waitingTests[i].id);

        appendPart(responseFile, "ended-tests-table-group-bottom");
        fprintf(responseFile, "</td></tr>");
    }

    appendPart(responseFile, "ended-tests-table-bottom");

    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Zwraca stronę z listą testów dla studenta
 * @param responseFile
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @param session
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetTestsByStudent(
        FILE *responseFile, ConnectionInfo connectionInfo, int messageType, char *message, Session session) {

    int count = 0;
    WaitingTest waitingTests[MAX_ARRAY] = {{0}};
    int errorCode = getWaitingTestsByStudentAndGroupAndNotEnded(waitingTests, &count, session.groupId, session.userId);
    if (errorCode != 0) {
        return errorCode;
    }

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "waiting-tests-top");
    appendPart(responseFile, "waiting-tests-table-top");

    for(int i = 0; i < count; i++) {
        fprintf(responseFile, "<tr><td>%d</td><td>%s</td><td>",
                (i + 1), waitingTests[i].subject);
        appendPart(responseFile, "waiting-tests-table-group-top");
        fprintf(responseFile, "<input type=\"hidden\" value=\"%d\" name=\"test\" id=\"test\">", waitingTests[i].id);
        appendPart(responseFile, "waiting-tests-table-group-bottom");
        fprintf(responseFile, "</td></tr>");
    }

    appendPart(responseFile, "waiting-tests-table-bottom");

    //Wyniki testów:

    int checkedCount = 0;
    WaitingTest checkedTests[MAX_ARRAY] = {{0}};
    errorCode = getCheckedTestByStudent(checkedTests, &checkedCount, session.userId);
    if (errorCode != 0) {
        return errorCode;
    }

    appendPart(responseFile, "results-top");
    if (messageType != NO_MESSAGE) appendMessage(responseFile, messageType, message);
    appendPart(responseFile, "results-table-top");

    for(int i = 0; i < checkedCount; i++) {
        fprintf(responseFile, "<tr><td>%d</td><td>%s</td><td>%d p.</td></tr>",
                (i + 1), checkedTests[i].subject, checkedTests[i].result);
    }

    appendPart(responseFile, "results-table-bottom");

    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Zwraca stronę z dodawaniem nowego testu
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToGetAddTest(ConnectionInfo connectionInfo, int messageType, char * message) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn || strcmp(session.role, "EGZAMINATOR") != 0) {
        return UNAUTHORIZED;
    }

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "add-test-top");
    if (messageType != NO_MESSAGE) appendMessage(responseFile, messageType, message);
    appendPart(responseFile, "add-test-form");
    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Dodaje nowy test
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostAddTest(ConnectionInfo connectionInfo) {
    int errors = 0;

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn || strcmp(session.role, "EGZAMINATOR") != 0) {
        return UNAUTHORIZED;
    }

    char subject[FIELD_LENGTH];
    errors += getPostParameterValue(connectionInfo.requestBody, "subject", subject, MANDATORY);
    if (errors != 0) {
        return respondToGetAddTest(connectionInfo, ALERT, "Brak tematu testu!");
    }

    int testId = 0;
    int errorCode = addTest(session.userId, subject, &testId);
    if (errorCode != 0) {
        return errorCode;
    }


    char query[BUFSIZ];
    for (int i = 1; i <= 10; ++i) {
        errors = 0;
        char newQuery[BUFSIZ];

        char question[LONG_FIELD_LENGTH];
        char answerA[FIELD_LENGTH];
        char answerB[FIELD_LENGTH];
        char answerC[FIELD_LENGTH];
        char answerD[FIELD_LENGTH];
        char correct[FIELD_LENGTH];

        char parameter[32];

        sprintf(parameter, "question%d", i);
        errors += getPostParameterValue(connectionInfo.requestBody, parameter, question, MANDATORY);

        sprintf(parameter, "answer%da", i);
        errors += getPostParameterValue(connectionInfo.requestBody, parameter, answerA, MANDATORY);

        sprintf(parameter, "answer%db", i);
        errors += getPostParameterValue(connectionInfo.requestBody, parameter, answerB, MANDATORY);

        sprintf(parameter, "answer%dc", i);
        errors += getPostParameterValue(connectionInfo.requestBody, parameter, answerC, MANDATORY);

        sprintf(parameter, "answer%dd", i);
        errors += getPostParameterValue(connectionInfo.requestBody, parameter, answerD, MANDATORY);

        sprintf(parameter, "correct%d", i);
        errors += getPostParameterValue(connectionInfo.requestBody, parameter, correct, MANDATORY);

        if (errors > 0 && i > 1) {
            break;
        } else if (errors > 0) {
            return respondToGetAddTest(connectionInfo, ALERT, "Niewłaściwe dane do testu (należy dodać co najmniej jedno pytanie)!");
        }

        sprintf(newQuery, INSERT_QUESTION, testId, question, answerA, answerB, answerC, answerD, correct);
        appendToQuery(query, newQuery);
    }

    errorCode = runPreparedQuery(query);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToGetAddTest(connectionInfo, SUCCESS, "Test został dodany");
}

/**
 * Zwraca stronę z rozwiązywaniem testu
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostTest(ConnectionInfo connectionInfo) {
    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn || strcmp(session.role, "STUDENT") != 0) {
        return UNAUTHORIZED;
    }

    int errors = 0;
    char test[FIELD_LENGTH];
    errors += getPostParameterValue(connectionInfo.requestBody, "test", test, MANDATORY);
    if (errors != 0) {
        return BAD_REQUEST;
    }

    int waitingTestId = atoi(test);
    WaitingTest waitingTest = {0};

    int errorCode = getWaitingTestByIdAndNotEnded(&waitingTest, waitingTestId);
    if (errorCode != 0) {
        return respondToGetTests(connectionInfo, ALERT, "Ten test już się zakończył!");
    } else if (errorCode > 0) {
        return errorCode;
    }

    int questionsCount = 0;
    Question questions[10];
    errorCode = getQuestionsByTestId(questions, &questionsCount, waitingTest.testId);
    if (errorCode != 0) {
        return errorCode;
    }

    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "test-top");
    fprintf(responseFile, "<form action=\"/sendTest\" method=\"POST\" accept-charset=\"UTF-8\">");
    fprintf(responseFile, "<input type=\"hidden\" value=\"%d\" name=\"test\" id=\"test\">", waitingTest.id);
    fprintf(responseFile, "<input type=\"hidden\" value=\"%d\" name=\"question-count\" id=\"question-count\">", questionsCount);

    for (int i = 0; i < questionsCount; ++i) {
        fprintf(responseFile, "<p class=\"mt-4\"><strong>%d. %s</strong></p>", (i+1), questions[i].question);

        fprintf(responseFile, "<div class=\"form-check form-check-inline\"><input class=\"form-check-input\" type=\"radio\" name=\"answer%d\" id=\"answer%da\" value=\"A\" checked><label class=\"form-check-label\" for=\"answer%da\">%s</label></div>", (i+1), (i+1), (i+1), questions[i].answer1);
        fprintf(responseFile, "<div class=\"form-check form-check-inline\"><input class=\"form-check-input\" type=\"radio\" name=\"answer%d\" id=\"answer%db\" value=\"B\"><label class=\"form-check-label\" for=\"answer%db\">%s</label></div>", (i+1), (i+1), (i+1), questions[i].answer2);
        fprintf(responseFile, "<div class=\"form-check form-check-inline\"><input class=\"form-check-input\" type=\"radio\" name=\"answer%d\" id=\"answer%dc\" value=\"C\"><label class=\"form-check-label\" for=\"answer%dc\">%s</label></div>", (i+1), (i+1), (i+1), questions[i].answer3);
        fprintf(responseFile, "<div class=\"form-check form-check-inline\"><input class=\"form-check-input\" type=\"radio\" name=\"answer%d\" id=\"answer%dd\" value=\"D\"><label class=\"form-check-label\" for=\"answer%dd\">%s</label></div>", (i+1), (i+1), (i+1), questions[i].answer4);
    }

    appendPart(responseFile, "test-form-bottom");
    appendPart(responseFile, "footer");

    fclose(responseFile);
    return 0;
}

/**
 * Wysyła odpowiedzi na test
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostSendTest(ConnectionInfo connectionInfo) {
    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn || strcmp(session.role, "STUDENT") != 0) {
        return UNAUTHORIZED;
    }

    int errors = 0;
    char test[FIELD_LENGTH];
    char count[FIELD_LENGTH];
    errors += getPostParameterValue(connectionInfo.requestBody, "test", test, MANDATORY);
    errors += getPostParameterValue(connectionInfo.requestBody, "question-count", count, MANDATORY);
    if (errors != 0) {
        return respondToGetTests(connectionInfo, ALERT, "Błąd wysyłania testu!");
    }

    int questionsCount = atoi(count);
    int waitingTestId = atoi(test);
    WaitingTest waitingTest = {0};

    int errorCode = getWaitingTestByIdAndNotEnded(&waitingTest, waitingTestId);
    if (errorCode != 0) {
        return respondToGetTests(connectionInfo, ALERT, "Ten test już się zakończył!");
    } else if (errorCode > 0) {
        return errorCode;
    }

    int i = 0;
    char answers[10];
    for (; i < questionsCount; i++) {
        char answer[10];
        char parameter[16];
        sprintf(parameter, "answer%d", i+1);
        errors += getPostParameterValue(connectionInfo.requestBody, parameter, answer, MANDATORY);
        if (errors != 0) {
            return respondToGetTests(connectionInfo, ALERT, "Błąd wysyłania testu!");
        }

        answers[i] = answer[0];
    }
    answers[i] = '\0';

    errorCode = getAnswersByWaitingTestIdAndStudentId(NULL, waitingTestId, session.userId);
    if (errorCode == 0) {
        return respondToGetTests(connectionInfo, ALERT, "Udzieliłeś już odpowiedzi na ten test!");
    } else if (errorCode > 0) {
        return errorCode;
    }

    errorCode = addAnswers(waitingTestId, session.userId, answers);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToGetTests(connectionInfo, SUCCESS, "Test został wysłany");
}

/**
 * Uruchamia automatyczne sprawdzanie testu
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostCheckTest(ConnectionInfo connectionInfo) {
    int errors = 0;

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    if (!session.loggedIn || strcmp(session.role, "EGZAMINATOR") != 0) {
        return UNAUTHORIZED;
    }

    char test[FIELD_LENGTH];
    errors += getPostParameterValue(connectionInfo.requestBody, "test", test, MANDATORY);
    if (errors != 0) {
        return respondToGetTests(connectionInfo, ALERT, "Brak parametru z numerem testu!");
    }

    int waitingTestId = atoi(test);

    WaitingTest waitingTest = {0};
    int errorCode = getWaitingTestById(&waitingTest, waitingTestId);
    if (errorCode == -1) {
        return respondToGetTests(connectionInfo, ALERT, "Brak takiego testu!");
    } else if (errorCode > 0) {
        return errorCode;
    }

    int questionsCount = 0;
    Question questions[10];
    errorCode = getQuestionsByTestId(questions, &questionsCount, waitingTest.testId);
    if (errorCode != 0) {
        return errorCode;
    }

    int answersCount = 0;
    Answers answers[MAX_ARRAY] = {{0}};
    errorCode = getAnswersByWaitingTestId(answers, &answersCount, waitingTest.id);
    if (errorCode != 0) {
        return errorCode;
    }

    int studentsInGroupCount = 0;
    int ids[MAX_ARRAY] = {0};
    errorCode = getStudentsIdsByGroup(ids, &studentsInGroupCount, waitingTest.groupId);
    if (errorCode != 0) {
        return errorCode;
    }

    int studentsOnExamCount = 0;
    int idsOfPresentStudents[MAX_ARRAY] = {0};
    char query[BUFSIZ];
    char newQuery[BUFSIZ];


    for (int i = 0; i < answersCount; ++i) {
        int total = 0;

        for (int j = 0; j < questionsCount; ++j) {
            char correct = questions[j].correct;
            char studentAnswer = answers[i].answers[j];

            if (correct == studentAnswer) total++;
        }

        sprintf(newQuery, "INSERT INTO result (waiting_test_id, student_id, result) VALUES (%d, %d, %d);",
                waitingTestId, answers[i].studentId, total);
        appendToQuery(query, newQuery);

        idsOfPresentStudents[studentsOnExamCount++] = answers[i].studentId;
    }

    for (int i = 0; i < studentsInGroupCount; ++i) {
        int notPresent = 1;
        for (int j = 0; j < studentsOnExamCount; ++j) {
            if (ids[i] == idsOfPresentStudents[j]) {
                notPresent = 0;
                break;
            }
        }

        if (notPresent == 1) {
            sprintf(newQuery, "INSERT INTO result (waiting_test_id, student_id, result) VALUES (%d, %d, 0);",
                    waitingTestId, ids[i]);
            appendToQuery(query, newQuery);
        }
    }

    errorCode = runPreparedQuery(query);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToGetTests(connectionInfo, SUCCESS, "Test dla grupy został sprawdzony");
}

/**
 * Dodaje nowe zapytanie do istniejącego zapytania
 * @param query istniejące zapytanie
 * @param newQuery nowe zapytanie
 */
void appendToQuery(char * query, char * newQuery) {
    char buffer[BUFSIZ];
    int length = strlen(query);

    if (length > 3) {
        sprintf(buffer, "%s%s", query, newQuery);
        strcpy(query, buffer);
    } else {
        strcpy(query, newQuery);
    }
}

/**
 *  Wyświetla na stronie wiadomość do użytkownika
 * @param responseFile otwarty do zapisu plik HTML
 * @param messageType typ wiadomości do użytkownika
 * @param message treść wiadomości do użytkownika
 */
void appendMessage(FILE * responseFile, int messageType, char * message) {
    switch (messageType) {
        case NO_MESSAGE:
            break;
        case SUCCESS:
            fprintf(responseFile, "<div class=\"alert alert-success\" role=\"alert\">%s</div>", message);
            break;
        case ALERT:
            fprintf(responseFile, "<div class=\"alert alert-danger\" role=\"alert\">%s</div>", message);
            break;
        default:
            break;
    }
}

/**
 * Wysyła dane logowania do serwera
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostLogin(ConnectionInfo connectionInfo) {
    int errors = 0;
    char login[MINI_BUF];
    char password[MINI_BUF];
    errors += getPostParameterValue(connectionInfo.requestBody, "login", login, 1);
    errors += getPostParameterValue(connectionInfo.requestBody, "password", password, 1);
    if (errors != 0) {
        return respondToGetLogin(connectionInfo, ALERT, "Brak wymaganych pól!");
    }

    User user;
    memset(&user, 0, sizeof(user));
    int errorCode = checkCredentials(login, password, &user);
    if (errorCode == -1) {
        return respondToGetLogin(connectionInfo, ALERT, "Niepoprawne dane logowania");
    }

    errorCode = saveSession(connectionInfo.sessionId, user.id, 1);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToIndex(connectionInfo, SUCCESS, "Zostałeś zalogowany");
}

/**
 * Wysyła do serwera żądanie o wylogowanie
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int respondToPostLogout(ConnectionInfo connectionInfo) {
    int errorCode =  saveSession(connectionInfo.sessionId, 0, 0);
    if (errorCode == -1) {
        return errorCode;
    }

    return respondToIndex(connectionInfo, SUCCESS, "Zostałeś wylogowany");
}

/**
 * Pobiera podany parametr z treści żądania POST
 * @param requestBody treść żądania POST
 * @param parameter parametr do znalezienia
 * @param value znaleziona wartość parametru
 * @param mandatory flaga określająca, czy parametr jest obowiązkowy
 * @return 0 w razie znalezienia lub jeśli parametr jest niewymagany,
 * 1 w razie nie znalezienia wymaganego parametru
 */
int getPostParameterValue(char * requestBody, char * parameter, char value[], int mandatory) {
    char * token;
    char buffer[BUFSIZ];
    strcpy(buffer, requestBody);
    int offset = strlen(parameter) + 1;
    char param[offset];
    sprintf(param, "%s=", parameter);

    token = strtok(buffer, "&");
    while (token != NULL) {
        if (strstr(token, param) && strlen(token) > offset) {
            strcpy(value, (token + offset));
            return 0;
        }
        token = strtok(NULL, "&");
    }
    if (mandatory) {
        return 1;
    }

    return 0;
}

/**
 * Dodaje do pliku html tresc pliku .part o podanej nazwie
 * (znajdujacego się w katalogu /web-content).
 * @param responseFile otwarty do zapisu plik HTML
 * @param part nazwa pliku (bez rozszerzenia)
 * @return 0 w razie sukcesu, 500 w razie błędu
 */
int appendPart(FILE *responseFile, char *part) {
    char path[SMALL_BUF];
    sprintf(path, "./web-content/%s.part", part);

    FILE *fileToSend = fopen(path, "r");
    if (fileToSend == NULL) {
        if (errno == ENOENT) {
            return NOT_FOUND;
        }

        return INTERNAL_SERVER_ERROR;
    }

    int c;
    while ((c = getc(fileToSend)) != EOF) {
        putc(c, responseFile);
    }

    fclose(fileToSend);
    return 0;
}

/**
 * Pobiera metodę z requestu
 * @param request treść requestu
 * @param method wynikowa metoda
 */
void getRequestMethod(char *request, char method[]) {
    char *token;
    char buffer[SMALL_BUF];
    strcpy(buffer, request);

    token = strtok(buffer, " ");
    strncpy(method, token, METHOD_LENGTH);
}

/**
 * Pobiera Argument z requestu.
 * @param request treść requestu
 * @param argument wynikowy argument
 */
void readArgumentFromRequest(char *request, char *argument) {
    char *token;
    char buffer[MINI_BUF];
    strcpy(buffer, request);

    strtok(buffer, " ");
    token = strtok(NULL, " ");

    sprintf(argument, "%s", token);
}

/**
 * Dodaje nagłówek do istniejących nagłówków
 * @param headers istniejące nagłówki
 * @param newHeader nowy nagłówek
 */
void appendToHeaders(char *headers, char *newHeader) {
    char buffer[BUFSIZ];
    if (strlen(headers) > 3) {
        sprintf(buffer, "%s\n%s", headers, newHeader);
    } else {
        sprintf(buffer, "%s", newHeader);
    }
    strncpy(headers, buffer, BUFSIZ);
}

/**
 * Przetwarza request o plik.
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int resolveGetFileRequest(ConnectionInfo connectionInfo) {
    char filename[MINI_BUF];
    readArgumentFromRequest(connectionInfo.request, filename);

    return sendFile(connectionInfo.connectionFd, filename, connectionInfo.headers);
}

/**
 * Wysyła podany plik do klienta
 * @param connectionFd
 * @param filename
 * @param headers
 * @return 0 w razie sukcesu lub odpowiedni błąd w razie jego wystąpienia
 */
int sendFile(int connectionFd, char * filename, char * headers) {
    FILE *responseFile = fdopen(connectionFd, "w");
    if (responseFile == NULL) {
        return INTERNAL_SERVER_ERROR;
    }

    char path[SMALL_BUF];
    sprintf(path, "./web-content%s", filename);

    FILE *fileToSend = fopen(path, "r");
    if (fileToSend == NULL) {

        if (errno == ENOENT) {
            return NOT_FOUND;
        }

        return INTERNAL_SERVER_ERROR;
    }

    fprintf(responseFile, "%s", ANSWER_OK);
    fprintf(responseFile, "%s\r\n\r\n", headers);

    int c;
    while ((c = getc(fileToSend)) != EOF) {
        putc(c, responseFile);
    }

    fclose(fileToSend);
    fclose(responseFile);
    return 0;
}

/**
 * Generuje nowe id sesji.
 * @param sessionId
 */
void generateNewSessionId(char *sessionId) {
    srand(time(NULL));
    char randomChars[SESSION_ID_LENGTH];
    for (int i = 0; i < SESSION_ID_LENGTH; i++) {
        int index = rand() % (int) (sizeof sessionChars - 1);
        randomChars[i] = sessionChars[index];
    }
    randomChars[SESSION_ID_LENGTH - 1] = '\0';
    strcpy(sessionId, randomChars);
}

/**
 * Tworzy strukturę z informacjami o requeście z podanych informacji
 * @param connectionFd identyfikator gniazda połączeniowego
 * @param request treść requestu
 * @param headers nagłówki
 * @param sessionId numer sesji
 * @param requestBody treść żądania POST
 * @return zwraca strukurę ConnectionInfo
 */
ConnectionInfo createConnectionInfo(int connectionFd, char *request, char *headers, char * sessionId, char * requestBody) {
    ConnectionInfo connectionInfo;
    connectionInfo.connectionFd = connectionFd;
    connectionInfo.request = request;
    connectionInfo.headers = headers;
    connectionInfo.sessionId = sessionId;
    connectionInfo.requestBody = requestBody;

    return connectionInfo;
}

/**
 * Generuje menu dla podanego użytkownika i jego roli.
 * @param responseFile otwarty do zapisu plik HTML
 * @param session informacje o sesji
 */
void appendMenu(FILE *responseFile, Session session) {
    fprintf(responseFile, "<ul class=\"navbar-nav\">");

    if (session.loggedIn && strcmp(session.role, "ADMIN") == 0)
        fprintf(responseFile, "<li class=\"nav-item\"><a class=\"nav-link active\" href=\"/addGroup\"><i class=\"fa fa-plus\" aria-hidden=\"true\"></i> Utwórz grupę</a></li>");

    if (session.loggedIn && strcmp(session.role, "ADMIN") == 0)
        fprintf(responseFile, "<li class=\"nav-item\"><a class=\"nav-link active\" href=\"/students\"><i class=\"fa fa-list-ol\" aria-hidden=\"true\"></i> Lista studentów</a></li>");

    if (session.loggedIn && strcmp(session.role, "EGZAMINATOR") == 0)
        fprintf(responseFile, "<li class=\"nav-item\"><a class=\"nav-link active\" href=\"/addTest\"><i class=\"fa fa-pencil-square-o\" aria-hidden=\"true\"></i> Utwórz test</a></li>");

    if (session.loggedIn && (strcmp(session.role, "EGZAMINATOR") == 0 || strcmp(session.role, "STUDENT") == 0))
        fprintf(responseFile, "<li class=\"nav-item\"><a class=\"nav-link active\" href=\"/tests\"><i class=\"fa fa-check-square-o\" aria-hidden=\"true\"></i> Moje testy</a></li>");

    fprintf(responseFile, "</ul><ul class=\"navbar-nav ml-auto\">");

    if (!session.loggedIn)
        fprintf(responseFile, "<li class=\"nav-item\"><a class=\"nav-link active\" href=\"/login\"><i class=\"fa fa-user\" aria-hidden=\"true\"></i> Zaloguj</a></li>");

    if (!session.loggedIn)
        fprintf(responseFile, "<li class=\"nav-item\"><a class=\"nav-link active\" href=\"/register\"><i class=\"fa fa-user-plus\" aria-hidden=\"true\"></i> Zarejestruj</a></li>");

    if (session.loggedIn)
        appendUserData(responseFile, session);


    fprintf(responseFile, "</ul></div></nav>");
}

/**
 * Dodaje do pliku html informacje o zalogowanym użytkowniku (imię, nazwisko, rola i grupa, jeśli przynależy)
 * @param responseFile otwarty do zapisu plik HTML
 * @param session informacje o sesji
 */
void appendUserData(FILE *responseFile, Session session) {
    fprintf(responseFile, "<li class=\"nav-item dropdown\"> <a class=\"nav-link dropdown-toggle\" href=\"#\" id=\"navbarDropdownMenuLink\" role=\"button\" data-toggle=\"dropdown\" aria-haspopup=\"true\" aria-expanded=\"false\"> <i class=\"fa fa-user-o\" aria-hidden=\"true\"></i> %s %s (%s)",
            session.firstName, session.lastName, session.role);

    if (session.groupId != 0) {
        fprintf(responseFile, " grupa %s", session.group);
    }

    fprintf(responseFile, " </a> <div class=\"dropdown-menu\" aria-labelledby=\"navbarDropdownMenuLink\"> <a class=\"dropdown-item\" href=\"/logout\">Wyloguj</a> </div></li>");
}

/**
 * Przetwarza błąd, jeśli wystąpił.
 * @param connectionInfo struktura z danymi o aktualnym requeście
 * @param errorCode kod błędu
 */
void handleError(ConnectionInfo connectionInfo, int errorCode) {
    FILE *responseFile = fdopen(connectionInfo.connectionFd, "w");

    Session session;
    memset(&session, 0, sizeof(session));
    getSession(connectionInfo.sessionId, &session);

    appendErrorHttpCode(responseFile, errorCode);
    fprintf(responseFile, "%s\r\n\r\n", connectionInfo.headers);

    appendPart(responseFile, "header");
    appendMenu(responseFile, session);
    appendPart(responseFile, "index-top");
    appendErrorMessage(responseFile, errorCode);
    appendPart(responseFile, "footer");

    fclose(responseFile);
}

/**
 * Zapisuje informacje o błędzie w plik HTML.
 * @param responseFile otwarty do zapisu plik HTML
 * @param errorCode kod błędu
 */
void appendErrorHttpCode(FILE * responseFile, int errorCode) {
    switch (errorCode) {
        case INTERNAL_SERVER_ERROR:
            fprintf(responseFile, "%s", INTERNAL_SERVER_ERROR_CODE);
            break;
        case NOT_FOUND:
            fprintf(responseFile, "%s", NOT_FOUND_CODE);
            break;
        case UNAUTHORIZED:
            fprintf(responseFile, "%s", UNAUTHORIZED_CODE);
            break;
        case BAD_REQUEST:
            fprintf(responseFile, "%s", BAD_REQUEST_CODE);
            break;
        default:
            exit(EXIT_FAILURE);
    }
}

/**
 *
 * @param responseFile otwarty do zapisu plik HTML
 * @param errorCode
 */
void appendErrorMessage(FILE * responseFile, int errorCode) {
    switch (errorCode) {
        case INTERNAL_SERVER_ERROR:
            fprintf(responseFile, "<h1>500 Internal Server Error</h1>");
            fprintf(responseFile, "<p>Nastąpił nieprzewidziany błąd aplikacji</p>");
            break;
        case NOT_FOUND:
            fprintf(responseFile, "<h1>404 Not found</h1>");
            fprintf(responseFile, "<p>Nie znaleziono podanej strony.</p>");
            break;
        case UNAUTHORIZED:
            fprintf(responseFile, "<h1>401 Unauthorized</h1>");
            fprintf(responseFile, "<p>Nie masz dostępu do tej strony.</p>");
            break;
        case BAD_REQUEST:
            fprintf(responseFile, "<h1>400 Bad Request</h1>");
            fprintf(responseFile, "<p>Coś robisz nie tak...</p>");
            break;
        default:
            exit(EXIT_FAILURE);
    }
}
