#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_SOCK_BUF    6

#define logd(fmt, ...) { printf("DEBUG: %s L %d : "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); }
#define logw(fmt, ...) { printf("WARN: %s L %d : "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); }
#define loge(fmt, ...) { printf("ERROR: %s L %d : "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); }

struct server_config {
    char * host;
    uint16_t port;
    int sock;
    struct sockaddr_in addr;
    int quit;
};

struct client_data {
    int sock;
    struct sockaddr addr;
};

struct server_config g_cfg;

int init_config()
{
    g_cfg.host = strdup("127.0.0.1");
    g_cfg.port = 8007;
}


void * proc_client(void * argv)
{
    struct client_data * client = (struct client_data *)argv;

    write(client->sock, "login:", 7);
    sleep(3);
    close(client->sock);

    // g_cfg.quit = 1;
    // close(g_cfg.sock);
    return NULL;
}


int start_client(int fd, struct sockaddr *addr)
{
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    struct client_data * client = (struct client_data *)malloc(sizeof(struct client_data));
    client->sock = fd;
    memcpy(&client->addr,  addr, sizeof(client->addr));

    pthread_create(&tid, &attr, proc_client, (void *)client);
    pthread_attr_destroy(&attr);

    return 0;
}

int start_server()
{
    struct sockaddr addr;
    socklen_t addrlen;

    /* if (inet_aton(g_cfg.host, &g_cfg.addr.sin_addr) == 0) {
        loge("inet_aton() error. %s\n", strerror(errno));
        return -1;
    }*/
    g_cfg.addr.sin_addr.s_addr = INADDR_ANY;
    g_cfg.addr.sin_family = AF_INET;
    g_cfg.addr.sin_port = htons(g_cfg.port);


    g_cfg.sock = socket(AF_INET, SOCK_STREAM, 0);
    if (g_cfg.sock == -1) {
        loge("socket() error. %s\n", strerror(errno));
        return -1;
    }


    int sockopt = 1;
    setsockopt(g_cfg.sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));

    if (bind(g_cfg.sock, (struct sockaddr *)&g_cfg.addr, sizeof(g_cfg.addr)) != 0) {
        loge("bind() error. %s\n", strerror(errno));
        return -1;
    }

    if (listen(g_cfg.sock, MAX_SOCK_BUF) != 0) {
        loge("listen() error. %s\n", strerror(errno));
        return -1;
    }

    while (! g_cfg.quit) {
        int s = accept(g_cfg.sock, &addr, &addrlen);
        if (s < 0) {
            logd("accept() fd=%d,  error: %s\n", s, strerror(errno));
        }
        else {
            logd("accept() fd: %d\n", s);
        }
        start_client(s, &addr);
    }

    return 0;
}

int main(int argc, char **argv)
{
    logd("start ...\n");
    init_config();

    start_server();

    return 0;
}

