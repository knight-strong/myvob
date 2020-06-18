/* includes  */
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define TRUE    1
#define FALSE   0


#define MAX_PACKAGE_SIZE        2048
#define BROADCAST_PORT          1900

#define MSG_ERR(fmt, arg...) printf(fmt, ##arg)

typedef void (* proc_udp_package_t)(struct sockaddr_in * addr_from, void * data, size_t data_len);

struct UDP_THREAD_PARAM {
        proc_udp_package_t package_proc;
};

static int s_done = 0;

/**
 * UDP广播
 *
 * @param[in] ip 接收者的ip地址
 * @param[in] port 端口
 * @param[in] data 发送的数据
 * @param[in] len  发送数据的长度
 */
int udp_broadcast(char *ip, int port, char *data, int len)
{
    int r = -1;   
    int sockfd = -1;
    struct sockaddr_in addr;
    int broadcast = 1;
    bzero(&addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    // create socket
    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) 
    {
        MSG_ERR("socket error. %s\n", strerror(errno));
        goto err_exit;
    }
    
    // broadcast opt.
    if ( setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
    {
        MSG_ERR("setsockopt error. %s \n", strerror(errno));
        goto err_exit;
    }
    
    // send data    
    while (TRUE)
    {
        if( sendto(sockfd, data, len, 0, (struct sockaddr *)&addr, sizeof(addr)) != len)
        {        
            if (errno == EINTR)
                continue;
            MSG_ERR("ip:%s, sendto error. %s\n", ip, strerror(errno));
            // 错误
            goto err_exit;
        }
        break;        
    };
    
    r = 0;
    
err_exit:
    if (sockfd != -1)
        close(sockfd);
    return r;
}

/**
 * 接收UDP广播数据
 */
void * udp_recvdata(void * argv)
{
    int r = -1;
    int sockfd = -1;
    int recvlen = 0;
    struct sockaddr_in addr;
    char data[MAX_PACKAGE_SIZE];
    size_t buf_len = sizeof(data);
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct sockaddr_in addr_from;

    struct UDP_THREAD_PARAM * param = (struct UDP_THREAD_PARAM *)argv;    
    proc_udp_package_t proc_udp_package_func = param->package_proc;
    
    memset(&addr, 0, sizeof(addr));
    memset(&addr_from, 0, sizeof(addr_from));    
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BROADCAST_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0); // IPPROTO_UDP
    if (sockfd == -1)
    {
        MSG_ERR("socket() err. %s", strerror(errno));
        goto err_exit;
    }

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        MSG_ERR("bind() err. %s", strerror(errno));
        goto err_exit;
    }

    while (!s_done)
    {
        while (TRUE)
        {
            // 接收数据报文
            recvlen = recvfrom(sockfd, data, buf_len, 0, (struct sockaddr *)&addr_from, &socklen);
            if (recvlen < 0)
            {
                if (errno == EINTR)
                    continue;
                MSG_ERR("recvfrom err. %s", strerror(errno));
                goto err_exit;
            }
            
            // 处理接收到的数据报
            proc_udp_package_func(&addr_from, data, recvlen);
            break;
        }
        // 处理完成一个报文后的处理
    }
    r = 0;
err_exit:
    if (sockfd != -1)
        close(sockfd);
    return NULL;
}

/**
 * 注册udp包接收后的处理函数
 *
 * @param[in] funcPtr udp包处理的回调函数
 * @return 线程ID
 */
pthread_t regist_udp_pack_proc(proc_udp_package_t funcPtr)
{
    static struct UDP_THREAD_PARAM param;    
    param.package_proc = funcPtr;
    
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, udp_recvdata, (void *)&param);
    pthread_attr_destroy(&attr);

    return tid;
}

void proc_recv(struct sockaddr_in * addr_from, void * data, size_t data_len)
{
    *((char *)data + data_len) = 0;
    printf("recv ... %s\n", (char *)data);
}

int main(int argc, char **argv)
{
    int r;
    int port = BROADCAST_PORT;
    // char *server = "172.16.43.255";
    char *server = "255.255.255.255";

    // char data[] = "hehj test broadcast.";

    printf("argc = %d\n", argc);
    if (argc > 1) {
        if (argc > 2) {
            server = argv[2];
        }
        printf("send to %s ...\n", server);
        char *data = argv[1];
        r = udp_broadcast(server, port, data, strlen(data));
        printf("send result: %d\n", r);
    }
    else {
        struct UDP_THREAD_PARAM param;
        param.package_proc = proc_recv;
        udp_recvdata(&param);
        // regist_udp_pack_proc(proc_recv);
    }
    return 0;
}

