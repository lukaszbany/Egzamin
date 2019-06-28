#include "AuthenticationProvider.h"

#define PASSWORD_LENGTH 256

int checkCredentials(char * login, char * password, User * user) {
    int errorCode = 0;

    errorCode = getUserByLogin(login, user);
    if (errorCode != 0) {
        return errorCode;
    }

    char decodedPassword[PASSWORD_LENGTH] = "";
    decodeChars(user->password, decodedPassword);

    if (strcmp(password, decodedPassword) == 0) {
        return 0;
    }

    return -1;
}