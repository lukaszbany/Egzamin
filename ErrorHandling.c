#include <bits/types/FILE.h>
#include <stdio.h>
#include <stdlib.h>
#include "ErrorHandling.h"

#define INTERNAL_SERVER_ERROR 500
#define INTERNAL_SERVER_ERROR_HTTP "HTTP/1.1 500 Internal Server Error"

void handleError(int connectionFd, int errorCode) {
    switch (errorCode) {
        case INTERNAL_SERVER_ERROR:
            internalServerError(connectionFd);
            break;
        default:
            exit(EXIT_FAILURE);
    }
}


void internalServerError(int connectionFd) {
    FILE * responseFile = fdopen(connectionFd, "w");
    if (responseFile == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    fprintf(responseFile, "%s\r\n", INTERNAL_SERVER_ERROR_HTTP);
    fprintf(responseFile, "Content-type: text/html\n");

    int c;
    FILE * responseBody = fopen( "./test.html" , "r");
    if ( responseBody != NULL )
    {
        fprintf(responseFile, "\r\n");
        while( (c = getc(responseBody) ) != EOF )
            putc(c, responseFile);
        fclose(responseBody);
        fclose(responseFile);
    }

    fclose(responseFile);

}
