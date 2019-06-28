#ifndef EGZAMIN_AUTHENTICATIONPROVIDER_H
#define EGZAMIN_AUTHENTICATIONPROVIDER_H

#include "libs/mydb.h"
#include "libs/mybase64.h"

int checkCredentials(char * login, char * password, User * user);

#endif //EGZAMIN_AUTHENTICATIONPROVIDER_H
