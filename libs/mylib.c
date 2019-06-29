#include "mylib.h"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <syslog.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

int isHex(int x);

/**
 * Dokonuje operacji niezbędnych, aby uruchomić tryb deamon.
 *
 * @param port port na którym działa aplikacja
 */
void runAsDaemon(int port) {
    daemon(0, 0);
    openlog("Egzamin", LOG_PID, LOG_USER);
}

/**
 * Tworzy gniazdo o podanym typie.
 * @param type typ połączenia
 * @return identyfikator pliku gniazda
 */
int createSocket(int type) {
    int fd = socket(PF_INET, type, 0);
    if (fd == -1) {
        printf("Error creating socket\n");
        syslog(LOG_ERR, "Error creating socket");
        exit(EXIT_FAILURE);
    }

    return fd;
}

/**
 * Tworzy gniazdo TCP
 * @return identyfikator pliku gniazda
 */
int createTcpSocket() {
    return createSocket(SOCK_STREAM);
}

struct sockaddr_in createSockaddrWithAddress(char *address, int port) {
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(address);
    servaddr.sin_port = htons(port);

    return servaddr;
}

/**
 * Tworzy strukturę sockaddr_in z podanym portem
 * @param port do struktury
 * @return gotową strukturę
 */
struct sockaddr_in createSockaddr(int port) {
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    return servaddr;
}

/**
 * Łączy gniazdo z portem.
 *
 * @param serverFd identyfikator gniazda serwera
 * @param port port do połączenia
 */
void bindSocketWithPort(int serverFd, int port) {
    struct sockaddr_in servaddr = createSockaddr(port);

    int result = bind(serverFd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (result == -1) {
        syslog(LOG_ERR, "Error binding socket.");
        exit(EXIT_FAILURE);
    }

    printf("Binded socket with port %d\n", port);
    syslog(LOG_INFO, "Binded socket with port %d\n", port);
}

/**
 * Zaczyna nasłuchiwanie na podanym gnieździe.
 *
 * @param serverFd identyfikator gniazda serwera
 * @param maxBacklog maksymalna liczba oczekujących połączeń
 */
void startListening(int serverFd, int maxBacklog) {
    int result = listen(serverFd, maxBacklog);
    if (result == -1) {
        syslog(LOG_ERR, "Error listening.");
        exit(EXIT_FAILURE);
    }
}

/**
 * Łączy gniazdo z portem i zaczyna nasłuchiwanie.
 * @param serverFd identyfikator gniazda serwera
 * @param port port do połączenia
 * @param maxBacklog maksymalna liczba oczekujących połączeń
 */
void bindAndListen(int serverFd, int port, int maxBacklog) {
    bindSocketWithPort(serverFd, port);
    startListening(serverFd, maxBacklog);
}

/**
 * Łączy z serwerem.
 *
 * @param connectionFd identyfikator gniazda połączenia
 * @param address adres serwera
 * @param port port na którym nastąpi połączenie
 */
void connectWithServer(int connectionFd, char * address, int port) {
    struct sockaddr_in servaddr = createSockaddrWithAddress(address, port);

    int result = connect(connectionFd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (result == -1) {
        printf("Error connecting with server %s on port %d.\n", address, port);
        syslog(LOG_ERR, "Error connecting with server %s on port %d.", address, port);
        exit(EXIT_FAILURE);
    }
}

/**
 * Tworzy gniazdo TCP i łączy z podanym adresem i portem.
 *
 * @param address do połączenia
 * @param port port na którym nastąpi połączenie
 * @return identyfikator gniazda połączenia
 */
int createTcpSocketAndConnect(char *address, int port) {
    int connectionFd = createTcpSocket();
    connectWithServer(connectionFd, address, port);

    return connectionFd;
}

/**
 * Pobiera nazwę hosta i umieszcza numer ip w parametrze ip.
 *
 * @param hostname nazwa hosta
 * @param ip wynikowe ip
 */
void getIp(char *hostname, char *ip) {
    int status;
    struct addrinfo hints, *p;
    struct addrinfo *servinfo;
    char ipstr[INET_ADDRSTRLEN];


    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    if ((status = getaddrinfo(hostname, NULL, &hints, &servinfo)) == -1) {
        printf("Error getting ip from hostname\n");
        syslog(LOG_ERR, "Error getting ip from hostname");
        exit(EXIT_FAILURE);
    }

    for (p=servinfo; p!=NULL; p=p->ai_next) {
        struct in_addr  *addr;
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv->sin_addr);
        }
        else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = (struct in_addr *) &(ipv6->sin6_addr);
        }
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    }

    strcpy(ip, ipstr);
    freeaddrinfo(servinfo);
}

/**
 * Pobiera nazwę hosta z adresu ip
 * @param address adres ip
 * @param hostname wynikowa nazwa hosta
 */
void getHostname(char * address, char * hostname) {
    struct sockaddr_in sockaddr = createSockaddrWithAddress(address, 0);
    socklen_t size = sizeof(sockaddr);

    char name[MAX_HOSTNAME_LENGTH];
    char service[MAX_HOSTNAME_LENGTH];
    if (getnameinfo((struct sockaddr*) &sockaddr, size, name, sizeof(name), service, sizeof(service), NI_NAMEREQD) != 0) {
        printf("Error getting hostname from ip\n");
        syslog(LOG_ERR, "Error getting hostname from ip");
        exit(EXIT_FAILURE);
    }

    strcpy(hostname, name);
}

/**
 * Odczytuje port z nazwy usługi.
 * @param given
 * @param protocol
 * @return
 */
int readPort(char * given, char * protocol) {
    char * serviceName;
    errno = 0;
    int port = (int) strtol(given, &serviceName, 10);
    if (*serviceName) {
        struct servent * servstruct = getservbyname(serviceName, protocol);
        if (servstruct == NULL) {
            printf("Error getting port by name %s!\n", serviceName);
            syslog(LOG_ERR, "Error getting port by name %s!\n", serviceName);
            exit(EXIT_FAILURE);
        }
        port = ntohs(servstruct->s_port);
    }

    return port;
}

/**
 * Odczytuje adres ip.
 * @param address
 * @param result
 */
void readIpAddress(char * address, char * result) {
    if (isIpAddress(address)) {
        strcpy(result, address);
    } else {
        getIp(address, result);
    }
}

/**
 * Sprawdza czy podany łańcuch znanków jest adresem ip.
 * @param address łancuch znaków do sprawdznia
 * @return 1 jeśli tak, 0 jeśli nie
 */
int isIpAddress(char * address) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, address, &(sa.sin_addr));
    return result;
}

/**
 * Przyjmuje połączenie od klienta.
 *
 * @param serverSocket identyfikator gniazda serwera
 * @return identyfikator gniazda klienta
 */
int acceptConnection(int serverSocket) {
    struct sockaddr_in clientaddr;
    socklen_t clientaddrSize = sizeof(clientaddr);

    int connectionFd = accept(serverSocket, (struct sockaddr *) &clientaddr, &clientaddrSize);
    if (connectionFd == -1) {
        printf("Error accepting connection!\n");
        syslog(LOG_ERR, "Error accepting connection!");
        exit(EXIT_FAILURE);
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientaddr.sin_addr), ip, INET_ADDRSTRLEN);
    printf("Accepted connection from %s\n", ip);
    syslog(LOG_INFO, "Accepted connection from %s\n", ip);

    return connectionFd;
}

/**
 * Przetwarza request do końca pliku, znajdując parametr SESSION_ID
 * w headerze Cookie i parametry zapytania POST.
 * @param requestHeaders Treść requestu
 * @param sessionId znalezione sessionId
 * @param requestBody znalezione parametry zapytania POST
 * @return 1 jeśli znaleziono sesję, 0 jeśli nie
 */
int processRequestData(FILE *requestHeaders, char *sessionId, char *requestBody) {
    int sessionFound = -1;
    int contentLength = 0;
    char	buf[BUFSIZ];
    char	cookie[BUFSIZ];

    while(fgets(buf, BUFSIZ, requestHeaders) != NULL && strcmp(buf,"\r\n") != 0) {
        if (strstr(buf, "Cookie: ")) {
            strncpy(cookie, (buf + 8), sizeof(cookie));
            sessionFound = findSessionIdInCookie(cookie, sessionId);
        }
        if (strstr(buf, "Content-Length: ")) {
            contentLength = atoi(buf + 16);
        }
    }

    if (contentLength > 0 && fgets(buf, (contentLength + 1), requestHeaders) != NULL) {
        char decodedBuf[BUFSIZ];
        decodeUrlEncoded(buf, decodedBuf);
        strcpy(requestBody, decodedBuf);
    }

    return sessionFound;
}

/**
 * Znajduje parametr SESSION_ID w nagłówku Cookie
 * @param cookie Cookie do przeszukania
 * @param sessionId znalezione SESSION_ID
 * @return 1 jeśli znajdzie, 0 jeśli nie
 */
int findSessionIdInCookie(char * cookie, char sessionId[]) {
    char * token;
    token = strtok(cookie, ";");
    while (token != NULL) {
        if (strstr(token, "SESSION_ID=")) {
            strncpy(sessionId, (token + 11), SESSION_ID_LENGTH);
            removeCRLF(sessionId);
            return 0;
        }
        token = strtok(NULL, ";");
    }

    return -1;
}

/**
 * Wycina znak nowej linii z łańcucha znakow
 * @param text tekst do trimowania
 */
void trim(char * text) {
    int lastChar = strlen(text) - 1;
    if (text[lastChar] == '\n')
        text[lastChar] = '\0';
}

/**
 * Wycina znaki powrotu karetki i nowej linii z łańcucha znakow
 * @param text tekst do trimowania
 */
void removeCRLF(char * text) {
    int lastChar = strlen(text);
    for (int i = 0; i < lastChar; ++i) {
        if (text[i] == '\n' || text[i] == '\r')
            text[i] = '\0';
    }
}

/**
 * Sprawdza czy wartość jest heksadecymalna
 * @param x
 * @return 1 jeśli tak, 0 jeśli nie
 */
int isHex(int x) {
    return	(x >= '0' && x <= '9')	||
              (x >= 'a' && x <= 'f')	||
              (x >= 'A' && x <= 'F');
}

/**
 * Dekoduje zakodowane parametry POST
 * @param encoded łańcuch wejściowy
 * @param decoded łańcuch zdekodowany
 * @return -1 jeśli błąd
 */
int decodeUrlEncoded(char *encoded, char *decoded) {
    char *o;
    const char *end = encoded + strlen(encoded);
    int c;

    for (o = decoded; encoded <= end; o++) {
        c = *encoded++;
        if (c == '+') c = ' ';
        else if (c == '%' && (	!isHex(*encoded++)	||
                                  !isHex(*encoded++)	||
                                  !sscanf(encoded - 2, "%2x", &c)))
            return -1;

        if (decoded) *o = c;
    }

    return o - decoded;
}

/**
 * Zwraca Timestamp bazy danych w postaci łańcucha znaków
 * dla czasu obecnego z przesunięciem o ilość minut podanych
 * jako parametr offsetInMinutes.
 *
 * @param timestamp wynikowy łańcuch znaków
 * @param offsetInMinutes minuty do przodu
 */
void createTimestamp(char * timestamp, int offsetInMinutes) {
    struct tm tm;
    time_t currentTime;
    time(&currentTime);
    currentTime += 60 * offsetInMinutes;

    localtime_r(&currentTime, &tm);

    sprintf(timestamp, "%d-%d-%d %d:%d:%d",
            (tm.tm_year + 1900),
            (tm.tm_mon + 1),
            tm.tm_mday,
            tm.tm_hour,
            (tm.tm_min),
            tm.tm_sec);
}
