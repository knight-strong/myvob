#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

int main(int argc, char** argv)
{
    int r;
    char *host = argv[1];
    struct addrinfo hint;
    struct addrinfo *result = NULL;

    memset(&hint, 0 , sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = 0;

    printf("host: %s\n", host);
    r = getaddrinfo(host, NULL, &hint, &result);


    while (result) {
        printf("r=%d, addrlen:%d, addr:%p, next:%p\n", r, result->ai_addrlen, result->ai_addr, result->ai_next);

        if (0) {
            struct sockaddr * addr = result->ai_addr;
            char *p = result->ai_addr->sa_data;
            printf("f(%d), %d.%d.%d.%d.%d.%d\n", addr->sa_family, 0xff & p[0], 0xff & p[1], 0xff & p[2], 0xff & p[3], 0xff & p[4], 0xff & p[5]);

        }

        struct sockaddr_in *in = (struct sockaddr_in *)result->ai_addr;
        char *p = (char *)&in->sin_addr;
        printf("%d.%d.%d.%d\n", 0xff & p[0], 0xff & p[1], 0xff & p[2], 0xff & p[3]);

        result = result->ai_next;
    }


    return 0;
}


