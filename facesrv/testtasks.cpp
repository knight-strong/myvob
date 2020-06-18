#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "tasks.h"
#include "simplelog.h"

static struct timeval s_time_begin;

#define TICK_BEGIN()    gettimeofday(&s_time_begin, 0)

#define TICK_PRINT(x)   { struct timeval tv; gettimeofday(&tv, 0); \
    logi(x" \t#### spend ms: %.2f\n", (tv.tv_sec - s_time_begin.tv_sec)*1000 + (tv.tv_usec - s_time_begin.tv_usec)*0.001); }

static void on_faceproc_proc(int pid, void *data) {
    sleep(1);
    // logi("on_faceproc_proc: pid=%d, data=%s\n", pid, (char *)data);
}

static void on_faceproc_exit(int pid) {
    // logi("on_faceproc_exit: pid=%d\n", pid);
}

static void on_faceproc_quit() {
    logi("on_faceproc_quit\n");
}

struct proc_config config = {
    .proc_numb = 16,
};

static int choose_proc(int proc_num, void *data) {
    static int cur = 0;
    if (++cur >= proc_num)
        cur = 0;
    return cur;
}

struct proc_callback cb = {
    .on_proc = on_faceproc_proc,
    .on_exit = on_faceproc_exit,
    .on_quit = on_faceproc_quit,
    .proc_choose = choose_proc,
};

/*
 * test
 */
int main(int argc, char **argv) {
    char *data = NULL;

        TICK_BEGIN();

    proc_init(&config, &cb);

    for (int i=0; i<3; i++) {
        data = (char *)malloc(256);
        sprintf(data, "data of task %d.", i);
        proc_task_post(data);
    }

    TICK_PRINT("[task posted]");

    logi("all task posted %s() l  %d\n", __func__, __LINE__);

    proc_quit();

    TICK_PRINT("[all done]");

    return 0;
}

