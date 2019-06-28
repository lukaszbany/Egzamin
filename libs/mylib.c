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

void runAsDaemon(int port) {
    daemon(0, 0);
    openlog("Egzamin", LOG_PID, LOG_USER);
}

int createSocket(int type) {
    int fd = socket(PF_INET, type, 0);
    if (fd == -1) {
        printf("Error creating socket\n");
        syslog(LOG_ERR, "Error creating socket");
        exit(EXIT_FAILURE);
    }

    return fd;
}

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

struct sockaddr_in createSockaddr(int port) {
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    return servaddr;
}

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

void startListening(int serverFd, int maxBacklog) {
    int result = listen(serverFd, maxBacklog);
    if (result == -1) {
        syslog(LOG_ERR, "Error listening.");
        exit(EXIT_FAILURE);
    }
}

void bindAndListen(int serverFd, int port, int maxBacklog) {
    bindSocketWithPort(serverFd, port);
    startListening(serverFd, maxBacklog);
}

void connectWithServer(int connectionFd, char * address, int port) {
    struct sockaddr_in servaddr = createSockaddrWithAddress(address, port);

    int result = connect(connectionFd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (result == -1) {
        printf("Error connecting with server %s on port %d.\n", address, port);
        syslog(LOG_ERR, "Error connecting with server %s on port %d.", address, port);
        exit(EXIT_FAILURE);
    }
}

int createTcpSocketAndConnect(char *address, int port) {
    int connectionFd = createTcpSocket();
    connectWithServer(connectionFd, address, port);

    return connectionFd;
}

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

void readIpAddress(char * address, char * result) {
    if (isIpAddress(address)) {
        strcpy(result, address);
    } else {
        getIp(address, result);
    }
}

int isIpAddress(char * address) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, address, &(sa.sin_addr));
    return result;
}

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
        strcpy(requestBody, buf);
    }

    return sessionFound;
}

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

void trim(char * text) {
    int lastChar = strlen(text) - 1;
    if (text[lastChar] == '\n')
        text[lastChar] = '\0';
}

void removeCRLF(char * text) {
    int lastChar = strlen(text);
    for (int i = 0; i < lastChar; ++i) {
        if (text[i] == '\n' || text[i] == '\r')
            text[i] = '\0';
    }
}