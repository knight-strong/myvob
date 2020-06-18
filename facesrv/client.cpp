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

#include "tasks.h"
#include "simplelog.h"
#include "storage.h"

#include "cxcore.h"
#include "highgui.h"
#include "cv.h"

#include "FiStdDefEx.h"
#include "THFeature_i.h"
#include "facesrv.h"
#include "client.h"

face_client::face_client()
{
    msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

    request.on_finish = on_task_finish;
    request.object = this;

    // pthread_mutex_init(&mutex, NULL);
    // sem_init(&sem, 0, 0);
}


face_client::~face_client()
{
    // rm msqid
    msgctl(msqid, IPC_RMID, NULL);
    // pthread_mutex_destroy(&mutex);
    // sem_destroy(&sem);
}

int face_client::peek_message(struct message *msg)
{
    if (msgrcv(msqid, msg, sizeof(struct message), 0, IPC_NOWAIT) < 0) {
        return -1;
    }
    return 0;
}

int face_client::get_message(struct message *msg)
{
    while (1) {
again_r1:
        if (msgrcv(msqid, msg, sizeof(struct message), 0, 0) < 0) {
            if (errno == EINTR)
                goto again_r1;
            else {
                loge("msgrcv error: %s\n", strerror(errno));
                return -1;
            }
        }
        return 0;
    }
    return 0;
}

int face_client::parse_result(struct message *msg, float *score)
{
    proc_task_data_facecomp *td = (proc_task_data_facecomp *)msg->data;
    *score = td->score;
    delete td;
    return 0;
}

int face_client::parse_result(struct message *msg, struct feature **fout)
{
    proc_task_data_feature_gen *td = (proc_task_data_feature_gen *)msg->data;
    feature_storage *s = (feature_storage *)td->custom;
    if (s) {
        if (td->feature) {
            struct feature *f = new feature;
            *fout = f;
            f->file = strdup(td->file);
            f->desc = NULL;
            f->size = glb_cfg.ef_size;

            f->data = td->feature;
            td->feature = NULL; // detach feature

            s->append_rec(f);
        }
        else {
            loge("##FAILED: %s\n", td->file);
        }

        s->num--;
        // logi("REQ_CMD_FEATURE_GEN, td:%p file={%s}, custom=%p done. %d left, count: %d\n", td, td->file, td->custom, s->num, s->get_count());
    }
    else {
        logi("REQ_CMD_FEATURE_GEN, td:%p file={%s}, custom=%p done\n", td, td->file, td->custom);
    }
    delete td;
    return 0;
}

int face_client::face_compare(const char *f1, const char *f2, float *score)
{
    struct message msg;
    proc_task_data_facecomp *td = new proc_task_data_facecomp();
    *score = -1;

    td->req = &request;
    if (td) {
        td->setFiles(f1, f2);
        proc_task_post(td);
        if (get_message(&msg) == 0) {
            return parse_result(&msg, score);
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }
    return 0;
}

void face_client::on_task_finish(void *req, proc_task_data *data)
{
    face_client *c = (face_client *)req;
    struct message msg;
    msg.code = 1,  // unused
    msg.data = data;

    // logi("on_task_finish(), %p\n", c);

again_w1:
    if (msgsnd(c->msqid, &msg, sizeof(struct message), 0) == 0) {
    }
    else {
        if (errno == EINTR)
            goto again_w1;
        else
            loge("send message error: %s\n", strerror(errno));
    }
}

int face_client::face_feature_gen(const char *path)
{
    DIR *dp;
    struct stat buf;
    char file[MAX_FILENAME];
    char idx[MAX_FILENAME];

    struct message msg;
    struct feature *f = NULL;
    int in = 0, out = 0;
    int r = -1;

    snprintf(idx, MAX_FILENAME, "%s/feature.idx", path);

    if ((dp = opendir(path)) == NULL) {
        loge("open directory %s failed:%s", path, strerror(errno));
        return -1;
    }

    feature_storage * storage = new feature_storage(idx);

    struct dirent* dirp = NULL;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 || strcmp(dirp->d_name, "feature.idx") == 0) {
            continue;
        }
        snprintf(file, MAX_FILENAME, "%s/%s", path, dirp->d_name);
        file[MAX_FILENAME - 1] = 0;
        if ((stat(file, &buf) == 0) && S_ISREG(buf.st_mode)) {
            // process the results  nowait
            if (in > out + (glb_cfg.proc_numb * MAX_MSG_PER_PROC)) {
                if (get_message(&msg) == 0) {
                    parse_result(&msg, &f);
                    out++;
                }
            }
            else {
                while (peek_message(&msg) == 0) {
                    parse_result(&msg, &f);
                    out++;
                }
            }

            proc_task_data_feature_gen * td = new proc_task_data_feature_gen();
            td->req = &request;
            td->set_file(file);
            td->custom = storage;
            proc_task_post(td);
            in++;
            storage->num++;
            // logi("[gen]: in %d / out %d\n", in, out);
            // logi("[post gen]: %s/%s, num: in %d, out %d\n", path, dirp->d_name, in, out);
        }
        else {
            logi("## ignore file: %s/%s\n", path, dirp->d_name);
        }
    }

    closedir(dp);

    r = 0;
    while (out < in) {
        if (get_message(&msg) < 0) {
            r = -1;
            break;
        }
        out++;
        parse_result(&msg, &f);
        // logi("[gen]: in %d / out %d\n", in, out);
        // logi("[gen out] %s  %d\n", f->file, out);
        if (out == in) {
            r = storage->save();
            logi("save to %s, r=%d\n", storage->filename(), r);
            break;
        }
    }
    storage->cleanup();
    delete storage;
    return r;
}

int face_client::parse_result(struct message *msg, feature_compare_result *rs)
{
    proc_task_data_feature_cmp *td = (proc_task_data_feature_cmp *)msg->data;
#if 0
    logi("compare rs: %f | %s | %s\n",
            td->score,
            td->features[0]->file,
            td->features[1]->file);
#endif
    feature_compare_result *r = new feature_compare_result;
    r->score = td->score;
    strcpy(r->f1, td->features[0]->file);
    strcpy(r->f2, td->features[1]->file);
    list_add_tail(&r->l, &rs->l);

    delete td;
    return 0;
}

feature_compare_result *face_client::face_features_compare(const char *idx1, const char *idx2)
{
    int in = 0, out = 0;
    feature_storage l1(NULL), l2(NULL);
    struct message msg;

    feature_compare_result *rs = new feature_compare_result;
    rs->l.next = rs->l.prev = &rs->l;

    l1.load(idx1);
    l2.load(idx2);

    logi("load done ...\n");

#if 0
    l1.dump();
    logi("dump 1 done\n");
    l2.dump();
    logi("dump 2 done\n");
#endif

    list_head *p1head = l1.get_head();
    list_head *p2head = l2.get_head();

    list_head *p1;
    list_for_each(p1, p1head) {
         struct feature *f1 = (struct feature *)p1;
         if (f1->size > 0) {
             list_head *p2;
             list_for_each(p2, p2head) {
                 // process the results  nowait
                 if (in > out + (glb_cfg.proc_numb * MAX_MSG_PER_PROC)) {
                     if (get_message(&msg) == 0) {
                         parse_result(&msg, rs);
                         out++;
                     }
                 }
                 else {
                     while (peek_message(&msg) == 0) {
                         parse_result(&msg, rs);
                         out++;
                     }
                 }

                 struct feature *f2 = (struct feature *)p2;
                 proc_task_data_feature_cmp *td = new proc_task_data_feature_cmp(f1, f2);
                 td->req = &request;
                 in++;
                 proc_task_post(td);
             }
         }
    }

    while (get_message(&msg) == 0) {
        parse_result(&msg, rs);
        out++;
        if (out == in) {
            break;
        }
    }

    l1.cleanup();
    l2.cleanup();

    return rs;
}

static int compare_lookup_result(struct list_head *a, struct list_head *b)
{
    return ((struct feature_lookup_result *)a)->score > ((struct feature_lookup_result *)b)->score;
}

struct feature_lookup_result *face_client::face_lookup(const char *file, feature_storage *s)
{
    int in = 0, out = 0;
    struct feature_lookup_result *rs = NULL;
    struct message msg;

    proc_task_data_feature_gen * td = new proc_task_data_feature_gen();
    td->req = &request;
    td->set_file(file);
    td->custom = s;
    proc_task_post(td);

    if (get_message(&msg) < 0) {
        delete s;
        delete td;
        return NULL;
    }

    struct feature *f = new feature;
    f->file = strdup(td->file);
    f->desc = NULL;
    f->size = glb_cfg.ef_size;
    f->data = td->feature;
    td->feature = NULL; // detach feature
    delete td;

    list_head *p;
    list_head *head = s->get_head();
    list_for_each(p, head) {
        struct feature *f1 = (struct feature *)p;
        if (f1->size > 0) {
            in++;
            proc_task_data_feature_cmp *t = new proc_task_data_feature_cmp(f1, f);
            t->req = &request;
            proc_task_post(t);
        }
    }

    rs = new feature_lookup_result;
    rs->l.next = rs->l.prev = &rs->l;

    while (out < in) {
        if (get_message(&msg) < 0) {
            break;
        }

        out++;
        proc_task_data_feature_cmp *t = (proc_task_data_feature_cmp *)msg.data;
#if 0
        logi("REQ_CMD_FEATURE_CMP: [score: %f], files:%s vs %s ####\n",
                t->score, t->features[0]->file, t->features[1]->file);

        logi("left %d tasks\n", in - out);
#endif
        struct feature_lookup_result *n = new feature_lookup_result;
        n->l.next = n->l.prev = &n->l;
        n->score = t->score;
        strcpy(n->f, t->features[0]->file);
        delete t;

        // list_add_tail(&n->l, &rs->l);
        list_insert_sort(&n->l, &rs->l, compare_lookup_result);
    }

    return rs;
}

struct feature_lookup_result *face_client::face_lookup(const char *file)
{
    return face_lookup(file, glb_cfg.s);
}

struct feature_lookup_result *face_client::face_lookup(const char *file, const char *idx)
{
    feature_storage *s = new feature_storage(NULL);

    if (s->load(idx) != 0) {
        loge("load %s failed. error: %s\n", idx, strerror(errno));
        delete s;
        return NULL;
    }

    feature_lookup_result *rs = face_lookup(file, s);

    s->cleanup();
    delete s;

    return rs;
}

