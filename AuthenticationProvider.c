#include "AuthenticationProvider.h"

/**
 * Maksymalna długość hasła.
 */
#define PASSWORD_LENGTH 256


/**
 * Sprawdza czy dane użytkownika znajdują się w bazie
 * łącznie z odkodowaniem jego hasła i zwraca go, jeśli tak.
 * @param login login użytkownika
 * @param password hasło użytkownika
 * @param user znaleziony użytkownika
 * @return 0 jeśli login i hasło się zgadzają, -1 jeśli nie
 */
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