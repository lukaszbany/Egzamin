#ifndef EGZAMIN_ERRORS_H
#define EGZAMIN_ERRORS_H

/**
 * Niewłaściwe zapytanie do serwera
 */
#define BAD_REQUEST 400
#define BAD_REQUEST_CODE "HTTP/1.1 400 Bad Request\r\n"

/**
 * Użytkownik nie ma dostępu do podanej akcji
 */
#define UNAUTHORIZED 401
#define UNAUTHORIZED_CODE "HTTP/1.1 401 Unauthorized\r\n"

/**
 * Podany zasób nie został znaleziony na serwerze
 */
#define NOT_FOUND 404
#define NOT_FOUND_CODE "HTTP/1.1 404 Not Found"

/**
 * Błąd wewnętrzny serwera
 */
#define INTERNAL_SERVER_ERROR 500
#define INTERNAL_SERVER_ERROR_CODE "HTTP/1.1 500 Internal Server Error\r\n"

#endif //EGZAMIN_ERRORS_H
