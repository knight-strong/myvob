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
#include "opencv2/opencv.hpp"
using namespace cv;

#include "FiStdDefEx.h"
#include "THFeature_i.h"
#include "facesrv.h"

// config
#define NUMBER_PROC_CHANNEL     4
struct proc_config glb_cfg = {
    .proc_numb  = NUMBER_PROC_CHANNEL,
    .ef_size    =   -1,
};
// end config

static struct timeval s_time_begin;

static pthread_mutex_t mutex_feature;

static int main_msgid;

#define TICK_BEGIN()    gettimeofday(&s_time_begin, 0)

#define TICK_PRINT(x)   { logi(x" \t#### spend ms: %.2f\n", spend_ms()); }

/* app state */
enum {
    CLIENT_SATE_IDLE,
    CLIENT_STATE_FEATURE_DIR_GEN,
    CLIENT_STATE_1_V_1,
    CLIENT_STATE_N_V_N,
    CLIENT_STATE_1_V_N,
};

int s_client_state = CLIENT_SATE_IDLE;

//-----------------------------------
// test
int loops = 10;
float *results_1vn = NULL;
int results_1vn_size = 0;
int num = 0;
feature_storage l1(NULL);
feature_storage l2(NULL);
//----------------------------------

static double spend_ms()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (tv.tv_sec - s_time_begin.tv_sec)*1000 + (tv.tv_usec - s_time_begin.tv_usec)*0.001;
}

void on_compare_finish(void *object, proc_task_data *data)
{
    struct message msg;
    msg.code = 1,  // unused
    msg.data = data;

again_w1:
    if (msgsnd(main_msgid, &msg, sizeof(struct message), 0) == 0) {
    }
    else {
        if (errno == EINTR)
            goto again_w1;
        else
            loge("send message error: %s\n", strerror(errno));
    }
}

struct proc_request compare_req = {
    .on_finish = on_compare_finish,
};

/*
 * test
 */
int test_compare_batch(int loops)
{
    logi("### proc_numb = %d, loops = %d ###\n", glb_cfg.proc_numb, loops);

    for (int i=0; i<loops; i++) {
        proc_task_data_facecomp *td = new proc_task_data_facecomp();
        td->req = &compare_req;
        if (td) {
            num++;
            td->setFiles(
#if 1
                    "/home/hehj/workspace/facedet/bin/photos/1-1.jpg",
                    "/home/hehj/workspace/facedet/bin/photos/1-2.jpg");
#else
            "/home/hehj/workspace/facedet/bin/photos/2-1.jpg",
                "/home/hehj/workspace/facedet/bin/photos/2-2.jpg");
#endif
            proc_task_post(td);
        }
        else {
            loge("out of memory !!!\n");
            exit(-1);
        }
    }

    TICK_PRINT("[task posted]");
    return 0;
}


static void proc_gen_feature_response(struct message &msg, int check_finish)
{
    proc_task_data_feature_gen *tg = (proc_task_data_feature_gen *)msg.data;
    TICK_PRINT("### ");
    feature_storage *s = (feature_storage *)tg->custom;
    if (s) {
        if (tg->feature) {
            struct feature *f = new feature;
            f->file = strdup(tg->file);
            f->desc = NULL;
            f->size = glb_cfg.ef_size;

            f->data = tg->feature;
            tg->feature = NULL; // detach feature

            s->append_rec(f);
        }
        else {
            loge("##FAILED: %s\n", tg->file);
        }

        s->num--;
        logi("REQ_CMD_FEATURE_GEN, tg:%p file={%s}, custom=%p done. %d left, count: %d\n", tg, tg->file, tg->custom, s->num, s->get_count());
        if (check_finish && s->num == 0) {
            s->save();
            logi("saved as %s\n", s->filename());
            TICK_PRINT("### ");
            s->cleanup();
            delete s;
            
            s_client_state = CLIENT_SATE_IDLE;
        }
    }
    else {
        logi("REQ_CMD_FEATURE_GEN, tg:%p file={%s}, custom=%p done\n", tg, tg->file, tg->custom);
    }
    delete tg;
}

static void proc_gen_feature_response_nowait()
{
    struct message msg;

    while (1) {
        if (msgrcv(main_msgid, &msg, sizeof(msg), 0, IPC_NOWAIT) < 0) {
            return;
        }

        proc_gen_feature_response(msg, 0);
    }
}


static int test_feature_gen(const char *path)
{
    DIR *dp;
    struct stat buf;
    char file[MAX_FILENAME];
    char idx[MAX_FILENAME];

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
#if 1
            // process the results  nowait
            proc_gen_feature_response_nowait();

            proc_task_data_feature_gen * tg = new proc_task_data_feature_gen();
            tg->req = &compare_req;
            tg->set_file(file);
            tg->custom = storage;
            proc_task_post(tg);
            storage->num++;
            logi("post gen feature: %s/%s, num: %d\n", path, dirp->d_name, storage->num);
#else
            // do directly in the main thread, bypass the tasks's threads
            proc_task_data_feature_gen * tg = new proc_task_data_feature_gen();
            tg->req = &compare_req;
            tg->set_file(file);
            tg->custom = storage;
            if (do_face_feature_gen(tg, 0) == 0) {
                static int n = 0;
                logi("[%d]gen ok . %s\n", ++n, file);
                delete tg;
            }
            else {
                loge("gen feature failed. %s\n", file);
            }
#endif
        }
        else {
            logi("## ignore file: %s/%s\n", path, dirp->d_name);
        }
    }

    closedir(dp);
    return 0;
}

static int test_feature_compare(const char *f1, const char *f2, feature_storage &l1, feature_storage &l2, int &num)
{
    l1.load(f1);
    l2.load(f2);

    l1.dump();
    l2.dump();

    list_head *p1head = l1.get_head();
    list_head *p2head = l2.get_head();

    list_head *p1;
    list_for_each(p1, p1head) {
         struct feature *f1 = (struct feature *)p1;
         if (f1->size > 0) {
             list_head *p2;
             list_for_each(p2, p2head) {
                 struct feature *f2 = (struct feature *)p2;
                 logi("todo: comp id %d, %d\n", f1->id, f2->id);
                 proc_task_data_feature_cmp *t = new proc_task_data_feature_cmp(f1, f2);
                 t->req = &compare_req;
                 num++;
                 proc_task_post(t);
             }
         }
    }
    return 0;
}

static int test_feature_lookup(const char *file, const char *idx)
{
    feature_storage *s = new feature_storage(NULL);
    if (s->load(idx) != 0) {
        loge("load %s failed. error: %s\n", idx, strerror(errno));
        return -1;
    }

    proc_task_data_feature_gen * tg = new proc_task_data_feature_gen();
    tg->req = &compare_req;
    tg->set_file(file);
    tg->custom = s;
    proc_task_post(tg);
    return 0;
}

static int proc_feature_1vn(struct message &msg, int &num)
{
    proc_task_data_feature_gen *tg = (proc_task_data_feature_gen *)msg.data;
    feature_storage *s = (feature_storage *)tg->custom;

    struct feature *f = new feature;
    f->file = strdup(tg->file);
    f->desc = NULL;
    f->size = glb_cfg.ef_size;

    f->data = tg->feature;
    tg->feature = NULL; // detach feature
    delete tg;

    list_head *p;
    list_head *head = s->get_head();
    num = 0;
    list_for_each(p, head) {
        struct feature *f1 = (struct feature *)p;
        if (f1->size > 0) {
            proc_task_data_feature_cmp *t = new proc_task_data_feature_cmp(f1, f);
            t->req = &compare_req;
            num++;
            proc_task_post(t);
        }
    }
    return 0;
}

static int state_message_handle_idle(struct message &msg)
{
    return 0;
}

static int state_message_handle_feature_dir(struct message &msg)
{
    if (((proc_task_data *)(msg.data))->get_req_cmd() == REQ_CMD_FEATURE_GEN) {
        proc_gen_feature_response(msg, 1);
    }
    else { loge("unexpected cmd %x in %s()\n", ((proc_task_data *)(msg.data))->get_req_cmd(), __func__);
    }

    return 0;
}

static int state_message_handle_1v1(struct message &msg)
{
    if (((proc_task_data *)(msg.data))->get_req_cmd() == REQ_CMD_FACE_COMPARE) {
        proc_task_data_facecomp *td = (proc_task_data_facecomp *)msg.data;
        if (td->result == 0) {
            logi("REQ_CMD_FACE_COMPARE, files={[%s], [%s]}, score: %f\n", td->files[0], td->files[1], td->score);
        }
        else {
            logi("REQ_CMD_FACE_COMPARE, files={[%s], [%s]}, ##FAIL##\n", td->files[0], td->files[1]);
        }
        TICK_PRINT("### ");

        delete td;

        if (--num == 0) {
            s_client_state = CLIENT_SATE_IDLE;
        }
    }
    else {
        loge("unexpected cmd %x in %s()\n", ((proc_task_data *)(msg.data))->get_req_cmd(), __func__);
    }
 
    return 0;
}

static int state_message_handle_NvN(struct message &msg)
{
    if (((proc_task_data *)(msg.data))->get_req_cmd() == REQ_CMD_FEATURE_CMP) {
        proc_task_data_feature_cmp *t = (proc_task_data_feature_cmp *)msg.data;
        logi("REQ_CMD_FEATURE_CMP: [%d vs %d][score: %f], files:%s vs %s ####\n",
                t->features[0]->id, t->features[1]->id, t->score,
                t->features[0]->file, t->features[1]->file);
        logi("left %d tasks\n", --num);
        if (num == 0) {
            l1.cleanup();
            l2.cleanup();

            s_client_state = CLIENT_SATE_IDLE;
        }
        delete t;
    }
    else {
        loge("unexpected cmd %x in %s()\n", ((proc_task_data *)(msg.data))->get_req_cmd(), __func__);
    }
    return 0;
}
 
static int state_message_handle_1vN(struct message &msg)
{
    switch (((proc_task_data *)(msg.data))->get_req_cmd()) {
        case REQ_CMD_FEATURE_GEN:
            {
                TICK_PRINT("[feature ok]");
                proc_feature_1vn(msg, num);
                results_1vn = new float[num];
                results_1vn_size = num;
                break;
            }

        case REQ_CMD_FEATURE_CMP:
            {
                proc_task_data_feature_cmp *t = (proc_task_data_feature_cmp *)msg.data;
                logi("REQ_CMD_FEATURE_CMP: [%d vs %d][score: %f], files:%s vs %s ####\n",
                        t->features[0]->id, t->features[1]->id, t->score,
                        t->features[0]->file, t->features[1]->file);
                logi("left %d tasks\n", --num);
                static int r_idx = 0;
                results_1vn[r_idx++] = t->score;
                if (num == 0) {
                    // todo: sort the score
                    TICK_PRINT("-------------[1vN results]---------");
                    for (int i=0; i<results_1vn_size; i++) {
                        logi("result: %f\n", results_1vn[i]);
                    }
                    logi("---------------[1vN done]-------\n");
                    TICK_PRINT("end");

                    s_client_state = CLIENT_SATE_IDLE;
                }
                delete t;
            }

        default:
            loge("unexpected cmd %x in %s()\n", ((proc_task_data *)(msg.data))->get_req_cmd(), __func__);
            break;
    }

    return 0;
}


int main(int argc, char **argv)
{
    char *path = "/home/hehj/workspace/facesrv/face/bin/mydb";
    struct message msg;
    pthread_mutex_init(&mutex_feature, NULL);

    if (argc > 1) {
        glb_cfg.proc_numb = atoi(argv[1]);
    }

    if (argc > 2) {
        loops = atoi(argv[2]);
    }

    if (argc > 3) {
        path = argv[3];
    }

    facesrv_start(&glb_cfg);

    TICK_BEGIN();

    main_msgid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);

    if (0) {
        // 1 vs 1
        s_client_state = CLIENT_STATE_1_V_1;
        test_compare_batch(loops);
    }

    if (1) {
        // generate the features of files in the directory
        logi("gen features for %s ...\n", path);
        s_client_state = CLIENT_STATE_FEATURE_DIR_GEN;
        test_feature_gen(path);
        TICK_PRINT("[test_feature_gen called]");
    }

    if (0) {
        // N vs N 
        const char *FILE1 = "/home/hehj/workspace/facesrv/face/bin/photos/feature.idx";
        const char *FILE2 = "/home/hehj/workspace/facesrv/face/bin/photos/feature.idx";
        s_client_state = CLIENT_STATE_N_V_N;
        test_feature_compare(FILE1, FILE2, l1, l2, num);
    }

    if (0) {
        // 1 vs N
        const char *file = "/home/hehj/workspace/facesrv/face/bin/photos/1-2.jpg";
        const char *idx = "/home/hehj/workspace/facesrv/face/bin/photos/feature.idx";
        s_client_state = CLIENT_STATE_1_V_N;
        test_feature_lookup(file, idx);
    }

    while (1) {
again_r1:
        if (msgrcv(main_msgid, &msg, sizeof(msg), 0, 0) < 0) {
            if (errno == EINTR)
                goto again_r1;
            else {
                loge("msgrcv error: %s\n", strerror(errno));
            }
        }

        switch (s_client_state) {
            case CLIENT_SATE_IDLE:
                state_message_handle_idle(msg);
                break;

            case CLIENT_STATE_FEATURE_DIR_GEN:
                state_message_handle_feature_dir(msg);
                break;
           
            case CLIENT_STATE_1_V_1:
                state_message_handle_1v1(msg);
                break;

            case CLIENT_STATE_N_V_N:
                state_message_handle_NvN(msg);
                break;

            case CLIENT_STATE_1_V_N:
                state_message_handle_1vN(msg);
                break;

            default:
                break;
        }
        
        if (s_client_state == CLIENT_SATE_IDLE) {
            logi("### done testing, exit ###\n");
            break;
        }
    }

    logi("[all done]### proc_numb = %d, loops = %d, ms = %.2f, msp: %f ###\n", glb_cfg.proc_numb, loops, spend_ms(), spend_ms() / loops);

    pthread_mutex_destroy(&mutex_feature);

    facesrv_stop();

    return 0;
}

