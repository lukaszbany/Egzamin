#include "RequestProcessor.h"

#define MINI_BUF 128
#define SMALL_BUF 256
#define METHOD_LENGTH 8
#define ANSWER_OK "HTTP/1.1 200 OK\r\n"

#define MANDATORY 1
#define OPTIONAL 2

#define NO_MESSAGE 0
#define SUCCESS 1
#define ALERT 2

const char sessionChars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPRSTUVWXYZ";

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
        errorCode = saveSessionData(sessionId, 0, 0);
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

int resolveRequest(ConnectionInfo connectionInfo) {
    if (isFileRequest(connectionInfo.request, connectionInfo.headers)) {
        return resolveGetFileRequest(connectionInfo);
    } else if (isMappedRequest(connectionInfo.request)) {
        return resolveMappedRequest(connectionInfo);
    }

    return NOT_FOUND;
}

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
    } else if (strcmp(argument, "/addTest") == 0) {
        return 1;
    } else if (strcmp(argument, "/createdTests") == 0) {
        return 1;
    } else if (strcmp(argument, "/waitingTests") == 0) {
        return 1;
    }

    return 0;
}

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
    }

    return NOT_FOUND;
}

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

int respondToPostRegister(ConnectionInfo connectionInfo) {
    int errors = 0;
    char login[MINI_BUF];
    char password[MINI_BUF];
    char firstName[MINI_BUF];
    char lastName[MINI_BUF];
    char role[MINI_BUF];
    errors += getPostParameterValue(connectionInfo.requestBody, "login", login, 1);
    errors += getPostParameterValue(connectionInfo.requestBody, "password", password, 1);
    errors += getPostParameterValue(connectionInfo.requestBody, "first-name", firstName, 1);
    errors += getPostParameterValue(connectionInfo.requestBody, "last-name", lastName, 1);
    errors += getPostParameterValue(connectionInfo.requestBody, "role", role, 1);
    if (errors != 0) {
        return respondToGetLogin(connectionInfo, ALERT, "Brak wymaganych pól!");
    }

    int loginIsFree = getUserByLogin(login, NULL);
    if (!loginIsFree) {
        return respondToGetRegister(connectionInfo, ALERT, "Użytownik o tym loginie już istnieje!");
    }

    char encodedPassword[MINI_BUF];
    encodeChars(password, encodedPassword);

    int errorCode = addUser(login, encodedPassword, firstName, lastName, role);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToIndex(connectionInfo, SUCCESS, "Zostałeś zarejestrowany. Teraz możesz się zalogować.");
}

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

    errorCode = saveSessionData(connectionInfo.sessionId, user.id, 1);
    if (errorCode != 0) {
        return errorCode;
    }

    return respondToIndex(connectionInfo, SUCCESS, "Zostałeś zalogowany");
}

int respondToPostLogout(ConnectionInfo connectionInfo) {
    int errorCode =  saveSessionData(connectionInfo.sessionId, 0, 0);
    if (errorCode == -1) {
        return errorCode;
    }

    return respondToIndex(connectionInfo, SUCCESS, "Zostałeś wylogowany");
}

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

void getRequestMethod(char *request, char method[]) {
    char *token;
    char buffer[SMALL_BUF];
    strcpy(buffer, request);

    token = strtok(buffer, " ");
    strncpy(method, token, METHOD_LENGTH);
}

void readArgumentFromRequest(char *request, char *argument) {
    char *token;
    char buffer[MINI_BUF];
    strcpy(buffer, request);

    strtok(buffer, " ");
    token = strtok(NULL, " ");

    sprintf(argument, "%s", token);
}

void appendToHeaders(char *headers, char *newHeader) {
    char buffer[BUFSIZ];
    if (strlen(headers) > 3) {
        sprintf(buffer, "%s\n%s", headers, newHeader);
    } else {
        sprintf(buffer, "%s", newHeader);
    }
    strncpy(headers, buffer, BUFSIZ);
}

int resolveGetFileRequest(ConnectionInfo connectionInfo) {
    char filename[MINI_BUF];
    readArgumentFromRequest(connectionInfo.request, filename);

    return sendFile(connectionInfo.connectionFd, filename, connectionInfo.headers);
}

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

ConnectionInfo createConnectionInfo(int connectionFd, char *request, char *headers, char * sessionId, char * requestBody) {
    ConnectionInfo connectionInfo;
    connectionInfo.connectionFd = connectionFd;
    connectionInfo.request = request;
    connectionInfo.headers = headers;
    connectionInfo.sessionId = sessionId;
    connectionInfo.requestBody = requestBody;

    return connectionInfo;
}

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

void appendUserData(FILE *responseFile, Session session) {
    fprintf(responseFile, "<li class=\"nav-item dropdown\"> <a class=\"nav-link dropdown-toggle\" href=\"#\" id=\"navbarDropdownMenuLink\" role=\"button\" data-toggle=\"dropdown\" aria-haspopup=\"true\" aria-expanded=\"false\"> <i class=\"fa fa-user-o\" aria-hidden=\"true\"></i> %s %s (%s) </a> <div class=\"dropdown-menu\" aria-labelledby=\"navbarDropdownMenuLink\"> <a class=\"dropdown-item\" href=\"/logout\">Wyloguj</a> </div></li>",
            session.firstName, session.lastName, session.role);
}

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
            fprintf(responseFile, "<h1>402 Unauthorized</h1>");
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
