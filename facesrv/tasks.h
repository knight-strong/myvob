#ifndef __tasks_h_
#define __tasks_h_

class feature_storage;

struct proc_config {
    int proc_numb;
    int ef_size;
    char *baseidx;
    feature_storage *s;
};

extern struct proc_config glb_cfg;

struct message {
    long   code;
    void*  data;
};

enum {
    MSG_NEW_TASK    = 0x10001,
    MSG_QUIT,
    MSG_USER_TASK   = 0x20000,
};

struct proc_callback {
    void (*on_proc) (int proc_id, void *data);
    void (*on_exit) (int proc_id);
    void (*on_quit) ();
    int (*proc_choose) (int proc_num, void *data);
};

void proc_init(struct proc_config *config, struct proc_callback *cb);
void proc_quit();
int proc_task_post(void *data);

#endif

