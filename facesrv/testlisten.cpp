#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "simplelog.h"

// test
static struct timeval s_time_begin;

#define TICK_BEGIN()    gettimeofday(&s_time_begin, 0)
#define TICK_PRINT(x)   { logi(x" \t#### spend ms: %.2f\n", spend_ms()); }

static double spend_ms()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec - s_time_begin.tv_sec)*1000 + (tv.tv_usec - s_time_begin.tv_usec)*0.001;
}
// end


const char UNIX_DOMAIN[] = "/tmp/facesrv.s";

static int num = 0;

static int send_request(int fd)
{
    char buf[2048];
    const char *f1 = "/home/hehj/workspace/facesrv/face/bin/photos/1-1.jpg";
    const char *f2 = "/home/hehj/workspace/facesrv/face/bin/photos/1-2.jpg";

    // CMP0,FILE0,FILE1
    int len = sprintf(buf, "CMP0,%s,%s", f1, f2) + 1;
    int r;
    int wl = 0;
    while (wl < len) {
        r = write(fd, buf, len);
        if (r < 0) {
            printf("[snd][%d]failed: %s\n", r, strerror(errno));
            return -1;
        }
        else {
            wl += r;
        }
    }
    printf("[snd][s:%d][%s][%d]\n", fd, buf, wl);

    char *p = buf;
    len = sizeof(buf);
    memset(buf, 0, len);
    while (1) {
        r = read(fd, p, len);
        if (r < 0) {
            fprintf(stderr, "failed,r=%d\n", r);
            break;
        }
        if (r == 0) {
            break;
        }
        len -= r;
        p += r;
    }
    *p = 0;
    printf("[%d]result[score]:%s\n", num++, buf);
    return r;
}

int main(int argc, char **argv)
{
    int ret = 0;
    int fd;
    struct sockaddr_un srv_addr;
    // signal(SIGCHLD,SIG_IGN);
    // signal(SIGINT, SIG_IGN);    
    //signal(SIGHUP, SIG_IGN);   
    signal(SIGQUIT, SIG_IGN);  
    signal(SIGPIPE, SIG_IGN);  
    //signal(SIGTTOU, SIG_IGN); 
    //signal(SIGTTIN, SIG_IGN);


    while (1) {
        fd = socket(PF_UNIX, SOCK_STREAM, 0);

        srv_addr.sun_family = AF_UNIX;
        strncpy(srv_addr.sun_path, UNIX_DOMAIN, sizeof(srv_addr.sun_path) - 1);

        ret = connect(fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
        if (ret == -1) {
            fprintf(stderr, "cannot connect to server socket (%s), err:(%s)", srv_addr.sun_path, strerror(errno));
            goto err_exit;
        }

        TICK_BEGIN();

        send_request(fd);

        TICK_PRINT("done");

        close(fd);
    };

err_exit:
    close(fd);

    return ret;
}

