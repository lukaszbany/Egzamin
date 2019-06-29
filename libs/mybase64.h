#ifndef EGZAMIN_MYBASE64_H
#define EGZAMIN_MYBASE64_H

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

void decodeChars(char *chars, char *decodedChars);
void encodeChars(char *chars, char *encodedChars);

#endif //EGZAMIN_MYBASE64_H
