#ifndef EGZAMIN_ERRORHANDLING_H
#define EGZAMIN_ERRORHANDLING_H

#define BAD_REQUEST 400
#define UNAUTHORIZED 402
#define NOT_FOUND 404
#define INTERNAL_SERVER_ERROR 500

void handleError(int connectionFd, int errorCode);
void internalServerError(int connectionFd);

#endif //EGZAMIN_ERRORHANDLING_H
