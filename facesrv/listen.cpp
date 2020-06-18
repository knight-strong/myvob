#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "cxcore.h"
#include "highgui.h"
#include "cv.h"

#include "FiStdDefEx.h"
#include "THFeature_i.h"
#include "facesrv.h"
#include "client.h"
#include "tasks.h"
#include "simplelog.h"
#include "storage.h"

#define VERBOSE     0

struct proc_config glb_cfg = {
    .proc_numb  =   4,
    .ef_size    =   -1,
    .baseidx    =   "/srv/www/mydb",
    .s          =   NULL,
};


/*
 * request:                         | response
 * 1v1:  CMP0,FILE0,FILE1           | <SCORE>  
 * NvN:  CMP1,IDX0,IDX1             | loop{<SCORE|DESC0|DESC1,>}
 * 1vN:  IDEN,FILE                  | loop{<SCORE|DESC,>}
 * GeF:  GENF,FILE                  | <0|errno>     // generate features idx
 *
 */

const char UNIX_DOMAIN[] = "/tmp/facesrv.s";

static void do_response_cmp0(char *buf, int fd);
static void do_response_cmp1(char *buf, int fd);
static void do_response_identify(char *buf, int fd);
static void do_response_genidx(char *buf, int fd);

static void * do_response(void *arg)
{
    char buf[2048];
    int fd = (long)arg;

    int len = read(fd, buf, sizeof(buf) - 1);
    buf[len] = 0;
#if VERBOSE
    logi("[rcv][%s][%d]\n", buf, len);
#endif

    // parse command
    if (buf[0] == 'C' && buf[1] == 'M' && buf[2] == 'P' && buf[3] == '0') {
        do_response_cmp0(buf, fd);
    }
    else if (buf[0] == 'C' && buf[1] == 'M' && buf[2] == 'P' && buf[3] == '1') {
        do_response_cmp1(buf, fd);
    }
    else if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == 'E' && buf[3] == 'N') {
        do_response_identify(buf, fd);
    }
    else if (buf[0] == 'G' && buf[1] == 'E' && buf[2] == 'N' && buf[3] == 'F') {
        do_response_genidx(buf, fd);
    }
    else {
        logi("unexpected cmd: [%s]\n", buf);
    }
    shutdown(fd, SHUT_RD);
    close(fd);
    return NULL;
}

static int create_thread_response(long fd)
{
    pthread_t tid;
    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int r = pthread_create(&tid, &attr, do_response, (void *)fd);
    if (r != 0) {
        loge("pthread_create error: %s\n", strerror(errno));
    }   
    pthread_attr_destroy(&attr);
    return r;
}
                    
static void signal_ignore()
{
    signal(SIGCHLD,SIG_IGN);
    // signal(SIGINT, SIG_IGN);    
    signal(SIGHUP, SIG_IGN);   
    signal(SIGQUIT, SIG_IGN);  
    signal(SIGPIPE, SIG_IGN);  
    signal(SIGTTOU, SIG_IGN); 
    signal(SIGTTIN, SIG_IGN);
}

static int load_idx_db()
{
    char file[MAX_FILENAME];
    struct stat sb;
    face_client c;
    feature_storage *s = NULL;

    sprintf(file, "%s/feature.idx", glb_cfg.baseidx);

    if (stat(file, &sb) != 0) {
        logi("idx NOT found, try generate it ...\n");
        if (c.face_feature_gen(glb_cfg.baseidx) != 0) {
            logi("generate idx failed.\n");
            return -1;
        }
    }
    s = new feature_storage(NULL);
    if (s->load(file) == 0) {
        glb_cfg.s = s;
        return 0;
    }
    else {
        delete s;
        return -1;
    }
}

static void do_response_cmp0(char *buf, int fd)
{
    char *f1 = buf + 5;
    int i = 0;
    while (f1[i++] != ',');
    f1[i-1] = 0;
    char *f2 = f1 + i;
    // logi("cmp0, <%s> vs <%s>\n", f1, f2);

    float score = -1;
    face_client c1;

    // logi("do compare ...\n");
    if (c1.face_compare(f1, f2, &score) == 0) {
        // logi("1 v 2 score: %f\n", score);
    }
    else {
        logi("compare failed\n");
    }

    int len = sprintf(buf, "%f", score) + 1;
    if (write(fd, buf, len) == len) {
#if VERBOSE
        logi("r:%s, len:%d\n", buf, len);
#endif
    }
    else {
        loge("write response failed\n");
    }
}

static void do_response_cmp1(char *buf, int fd)
{
    // * NvN:  CMP1,IDX0,IDX1             | loop{<SCORE|DESC0|DESC1,>}
    char *f1 = buf + 5;
    int i = 0;
    while (f1[i++] != ',');
    f1[i-1] = 0;
    char *f2 = f1 + i;
    logi("cmp1, <%s> vs <%s>\n", f1, f2);

    face_client c;
    struct feature_compare_result *rs = c.face_features_compare(f1, f2);
#if VERBOSE
    logi("dump NvN result:------------------------------\n");
    i = 0;
#endif
    if (rs) {
        list_head *pl;
        list_for_each(pl, &rs->l) {
            struct feature_compare_result *p = (struct feature_compare_result *)pl;

            int len = sprintf(buf, "%f|%s|%s,", p->score, p->f1, p->f2);
            if (write(fd, buf, len) != len) {
                loge("write response failed\n");
                break;
            }

#if VERBOSE
            logi("compare rs:[%d] %f | %s | %s\n",
                    i++, p->score, p->f1, p->f2);
#endif
        }
    }
#if VERBOSE
    logi("-------<result end>--------------------\n");
#endif
}

static void do_response_identify(char *buf, int fd)
{
    // * 1vN:  IDEN,FILE                  | loop{<SCORE,DESC>}
    char *f = buf + 5;
    face_client c;

#if VERBOSE
    int i = 0;
    logi("identify %s\n", f);
#endif

    feature_lookup_result *rs = c.face_lookup(f);
#if VERBOSE
    logi("dump lookup result: [%s]--------------------------\n", f);
#endif
    if (rs) {
        list_head *pl;
        list_for_each(pl, &rs->l) {
            struct feature_lookup_result *p = (struct feature_lookup_result *)pl;
#if VERBOSE
            logi("compare rs:[%d] %f | %s\n", i++, p->score, p->f);
#endif
            int len = sprintf(buf, "%f|%s,", p->score, p->f);
            if (write(fd, buf, len) != len) {
                loge("write response failed\n");
                break;
            }
        }
    }
#if VERBOSE
    logi("-------<result end>--------------------\n");
#endif
}


static void do_response_genidx(char *buf, int fd)
{
    // * GeF:  GENF,FILE                  | <0|errno>     // generate features idx
    char *f = buf + 5;

    char file[MAX_FILENAME];
    struct stat sb;
    face_client c;
    feature_storage *s = NULL;

    sprintf(file, "%s/feature.idx", f);

    if (stat(file, &sb) != 0) {

        if (write(fd, "do ...", 7) != 7) {
            loge("write GENF response failed\n");
        }
        shutdown(fd, SHUT_RD);
        close(fd);

        logi("idx NOT found, try generate it ...\n");
        if (c.face_feature_gen(f) != 0) {
            logi("generate idx failed.\n");
        }
    }
    else {
        if (write(fd, "1", 1) != 1) {
            loge("write GENF response failed\n");
        }
    }
}
 
static int parse_commandline(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "n:d:")) != -1) {
        switch (opt) {
            case 'n':
                glb_cfg.proc_numb = atoi(optarg);
                break;
            case 'd':
                glb_cfg.baseidx = strdup(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-n thread numbers] [-d data path]\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    logi("proc: %d, datapath: %s\n", glb_cfg.proc_numb, glb_cfg.baseidx);
    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    long listen_fd, com_fd;
    socklen_t len;
    struct sockaddr_un srv_addr, clt_addr;

    signal_ignore();

    parse_commandline(argc, argv);

    facesrv_start(&glb_cfg);

    if (load_idx_db() != 0) {
        perror("[ERROR]create idx db failed.\n");
        exit(-1);
    }
    logi("start listen ...\n");

    listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);

    srv_addr.sun_family = AF_UNIX;
    strncpy(srv_addr.sun_path, UNIX_DOMAIN, sizeof(srv_addr.sun_path) - 1);
    unlink(UNIX_DOMAIN);

    ret = bind(listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if (ret == -1) {
        fprintf(stderr, "cannot bind server socket (%s), err:(%s)", srv_addr.sun_path, strerror(errno));
        goto err_exit;
    }

    ret = listen(listen_fd, 1);
    if(ret == -1) {
        perror("cannot listen the client connect request");
        goto err_exit;
    }

    chmod(UNIX_DOMAIN, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH);

    len = sizeof(clt_addr);
    while(1) {
        com_fd = accept(listen_fd, (struct sockaddr *)&clt_addr, &len);
        if (com_fd < 0) {
            perror("cannot accept client connect request");
            sleep(1);
            continue;
            // goto err_exit;
        }

        if (create_thread_response(com_fd) != 0) {
            loge("failed to create thread to response\n");
        }
    }

err_exit:
    close(listen_fd);
    unlink(UNIX_DOMAIN);

    return ret;
}


