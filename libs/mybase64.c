#include "mybase64.h"

#define CMD_LENGTH 120
#define HASH_LENGTH 120

void trimText(char *text);
void runCommand(char *encodedChars, const char *cmd);

/**
 * Wycina znak nowej linii z końca tekstu
 * @param text
 */
void trimText(char *text) {
    int lastChar = strlen(text) - 1;
    if (text[lastChar] == '\n')
        text[lastChar] = '\0';
}

/**
 * Uruchamia komendę systemową.
 *
 * @param encodedChars parametr
 * @param cmd komenda
 */
void runCommand(char *encodedChars, const char *cmd) {
    FILE *file = popen(cmd, "r");
    if (!file) {
        syslog(LOG_ERR, "Blad dekodowania hasla");
        exit(EXIT_FAILURE);
    }

    fgets(encodedChars, HASH_LENGTH, file);
    trimText(encodedChars);

    if (pclose(file) == -1) {
        printf("Blad zamykania pliku.");
        exit(EXIT_FAILURE);
    }
}

/**
 * Odkodowuje podane znaki zakodowane metodą base64.
 * @param chars zakodowane znaki
 * @param decodedChars odkodowane znaki
 */
void decodeChars(char *chars, char *decodedChars) {
    char cmd[CMD_LENGTH];
    sprintf(cmd, "echo -n %s | base64 --decode", chars);

    runCommand(decodedChars, cmd);
}

/**
 * Zakodowuje podane znaki metodą base64.
 * @param chars odkodowane znaki
 * @param encodedChars zakodowane znaki
 */
void encodeChars(char *chars, char *encodedChars) {
    char cmd[CMD_LENGTH];
    sprintf(cmd, "echo -n %s | base64", chars);

    runCommand(encodedChars, cmd);
}
