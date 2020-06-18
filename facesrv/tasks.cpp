#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>

#include "simplelog.h"
#include "tasks.h"

enum {
    STATE_IDLE      = 0,
    STATE_BUSY      = 1 
};

struct proc_info {
    int id;
    pthread_t thread;
    int state;
    int msgid;
};

static struct proc_config *cfg = NULL;

static struct proc_info *procs = NULL;

static struct proc_callback *callback = NULL;

static sem_t s_sem_proc;
static pthread_mutex_t s_mutex;

static void * thread_process(void *arg) {
    struct proc_info *proc = (struct proc_info *)arg;

    struct message msg; 
    while (1) {
        // logi("tracert %s() l %d\n", __func__, __LINE__);
again_r1:
        if (msgrcv(proc->msgid, &msg, sizeof(msg), 0, 0) < 0) {
            if (errno == EINTR)
                goto again_r1;
            else {
                loge("msgrcv error: %s\n", strerror(errno));
            }
        }

        // process the message
        if (msg.code == MSG_NEW_TASK) {
            // logi("proc [%d], data [%p], %s\n", proc->id, msg.data, (char *)msg.data);
            if (callback)
                callback->on_proc(proc->id, msg.data);
        }
        else if (msg.code == MSG_QUIT) {
            // logi("proc quit [%d] ...\n", proc->id);
            if (callback && callback->on_exit)
                callback->on_exit(proc->id);
            break;
        }
        else {
            loge("unexpected msg 0x%08lx", msg.code);
        }
    }
    return NULL;
}

static int create_thread_per_channel(struct proc_info *proc) {
    pthread_t tid;
    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    // joinable
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    int r = pthread_create(&tid, &attr, thread_process, proc);
    if (r != 0) {
        loge("pthread_create error: %s\n", strerror(errno));
    }
    proc->thread = tid;
    pthread_attr_destroy(&attr);
    return r;
}

static struct proc_info* get_proc(void *data) {
    if (callback && callback->proc_choose) {
        return &procs[callback->proc_choose(cfg->proc_numb, data)];
    }
    else {
        static int cur = 0;
#if 1
        // lock for changing cur
        pthread_mutex_lock(&s_mutex);

        // loop for balance
        struct proc_info* r = &procs[cur];

        if (++cur >= cfg->proc_numb)
            cur = 0;

        pthread_mutex_unlock(&s_mutex);

        return r;
#else
        return &procs[random() % cfg->proc_numb];
#endif
    }
}

int proc_task_post(void *data) {
    struct proc_info * p = get_proc(data);
    if (!p) {
        loge("get_proc failed\n");
        return -1;
    }

    // post to thread
    struct message msg;
    msg.code = MSG_NEW_TASK;
    msg.data = data;
again_w1:
    if (msgsnd(p->msgid, &msg, sizeof(struct message), 0) == 0) {
        return 0;
    }
    else {
        if (errno == EINTR)
            goto again_w1;
        else
            loge("send message error: %s\n", strerror(errno));
    }
    return -1;
}

void proc_init(struct proc_config *config, struct proc_callback *cb) {
    cfg = config;
    callback = cb;
    size_t size = cfg->proc_numb * sizeof(struct proc_info);
    procs = (struct proc_info *)malloc(size);
    memset(procs, 0, size);

    for (int i=0; i<cfg->proc_numb; i++) {
        procs[i].id = i;
        procs[i].msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

        logi("create proc[%d] ...\n", i);
        create_thread_per_channel(&procs[i]);
    }

    // init others
    sem_init(&s_sem_proc, 0, 0);
    pthread_mutex_init(&s_mutex, NULL);
}

void proc_quit() {
    struct message msg = {
        .code = MSG_QUIT,
        .data = NULL,
    };

    for (int i=0; i<cfg->proc_numb; i++) {
again_w1:
        if (msgsnd(procs[i].msgid, &msg, sizeof(struct message), 0) != 0) {
            if (errno == EINTR)
                goto again_w1;
            else
                loge("send message error: %s\n", strerror(errno));
        }

        if (pthread_join(procs[i].thread, NULL) != 0) {
            loge("pthread_join error: %s\n", strerror(errno));
        }
    }

    if (callback)
        callback->on_quit();

    for (int i=0; i<cfg->proc_numb; i++) {
        msgctl(procs[i].msgid, IPC_RMID, NULL);
    }
    free(procs);

    // others cleanup
    pthread_mutex_destroy(&s_mutex);
    sem_destroy(&s_sem_proc);
}

