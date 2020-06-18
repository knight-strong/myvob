#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "tasks.h"
#include "simplelog.h"

#include "cxcore.h"
#include "highgui.h"
#include "cv.h"
#include "opencv2/opencv.hpp"
using namespace cv;

#include "FiStdDefEx.h"
#include "THFeature_i.h"
#include "facesrv.h"
#include "storage.h"

int do_face_compare(proc_task_data_facecomp *td, int ch)
{
    int r = -1;
    THFI_FacePos fps[2];
    Mat m[2];
    BYTE *features[2] = {NULL, NULL};

    td->score = -1;

    m[0] = imread(td->files[0]);
    if (m[0].empty()) {
        loge("imread() error:%s\n", td->files[0]);
        goto exit_error;
    }

    m[1] = imread(td->files[1]);
    if (m[1].empty()) {
        loge("imread() error:%s\n", td->files[1]);
        goto exit_error;
    }

    for (int i=0; i<=1; i++) {
        r = THFI_DetectFace(ch, m[i].data, 24, m[i].cols, m[i].rows, &fps[i], 1, 0);
        if (r <= 0) {
            loge("THFI_DetectFace() error: %d\n", r);
            r = -1;
            goto exit_error;
        }
    }

    features[0] = new BYTE[glb_cfg.ef_size];
    features[1] = new BYTE[glb_cfg.ef_size];

    for (int i=0; i<=1; i++) {
        r = EF_Extract(ch, m[i].data, m[i].cols, m[i].rows, 3, &fps[i], features[i]);
        if (r != 1) {
            loge("EF_Extract() error: %d\n", r);
            r = -1;
            goto exit_error;
        }
    }

    td->score = EF_Compare(features[0], features[1]);

    r = 0;

exit_error:
    for (int i=0; i<=1; i++) {
        if (features[i])
            delete[] features[i];
    }

    return r;
}

int do_face_feature_gen(proc_task_data_feature_gen *td, int ch)
{
#if 1
    int r = -1;
    THFI_FacePos fps;
    Mat m;
    BYTE *feature = NULL;

    m = imread(td->file);
    if (m.empty()) {
        loge("imread() error: %s\n", td->file);
        goto exit_error;
    }

    r = THFI_DetectFace(ch, m.data, 24, m.cols, m.rows, &fps, 1, 0);
    if (r <= 0) {
        loge("THFI_DetectFace() error: %d\n", r);
        r = -1;
        goto exit_error;
    }

    feature = new BYTE[glb_cfg.ef_size];

    r = EF_Extract(ch, m.data, m.cols, m.rows, 3, &fps, feature);
    if (r != 1) {
        loge("EF_Extract() error: %d\n", r);
        r = -1;
        goto exit_error;
    }

    r = 0;

exit_error:
    if (r && feature) {
        delete[] feature;
        td->feature = NULL;
    }
    else {
        td->feature = feature;
    }

    return r;
#else
    // test tasks performance
    // usleep(10000);

    // Mat m = imread(td->file);
    return -1;
#endif
}

inline void do_face_feature_cmp(proc_task_data_feature_cmp *t, int ch)
{
    t->score = EF_Compare((BYTE *)t->features[0]->data, (BYTE *)t->features[1]->data);
}

int facesrv_core_init(struct proc_config* cfg)
{
    int r;

    THFI_Param detParam;
    detParam.nMinFaceSize   = 50;
    detParam.nRollAngle     = 60;
    r = THFI_Create(cfg->proc_numb, &detParam);
    if (r < 0) {
        loge("THFI_Create failed!(ret=%d)\n", r);
        goto error_exit;
    }

    r = EF_Init(cfg->proc_numb);
    if (r < 0) {
        loge("EF_Init failed!(ret=%d)\n", r);
        goto error_exit;
    }

    cfg->ef_size = EF_Size();
    if (cfg->ef_size <= 0) {
        loge("EF_SIZE() failed!(%d)\n", cfg->ef_size);
        goto error_exit;
    }

    r = 0;
error_exit:
    return r;
}

void facesrv_core_uninit()
{
    THFI_Release();
    EF_Release();
}

void dump_hex(const char *p, int l)
{
    printf("dump [");
    for (int i=0; i<l; i++) {
        printf("%02x", p[i] & 0xff);
    }
    printf("]\n");
}

void on_faceproc_proc(int pid, void *data)
{
    proc_task_data * d = (proc_task_data *)data;
    // logi("on_faceproc_proc ... %d, cmd: 0x%x, %p\n", pid, d->get_req_cmd(), d);

    switch (d->get_req_cmd()) {
        case REQ_CMD_FACE_COMPARE:
            {
                proc_task_data_facecomp *td = (proc_task_data_facecomp *)d;
                td->result = do_face_compare(td, pid);

                if (td->req) {
                    td->req->on_finish(td->req->object, td);
                }
                else {
                    delete d;
                }
                break;
            }
        case REQ_CMD_FEATURE_GEN:
            {
                proc_task_data_feature_gen *td = (proc_task_data_feature_gen *)d;
                if (do_face_feature_gen(td, pid) == 0) {
                }
                else {
                    logi("REQ_CMD_FEATURE_GEN, pid:%d, file={%s} ##FAIL##\n", pid, td->file);
                }

                if (td->req) {
                    td->req->on_finish(td->req->object, td);
                }
                else {
                    delete d;
                }
                break;
            }
        case REQ_CMD_FEATURE_CMP:
            {
                proc_task_data_feature_cmp *td = (proc_task_data_feature_cmp *)d;
                do_face_feature_cmp(td, pid);

                if (td->req) {
                    td->req->on_finish(td->req->object, td);
                }
                else {
                    delete d;
                }

                break;
            }
        default:
            break;
    }
}

static void on_faceproc_exit(int pid)
{
    logi("on_faceproc_exit: pid=%d\n", pid);
}

static void on_faceproc_quit()
{
    logi("on_faceproc_quit\n");
}

/*
static int faceproc_choose_proc(int proc_num, void *data) {
    static int cur = 0;
    if (++cur >= proc_num)
        cur = 0;
    return cur;
}*/

static struct proc_callback cb = {
    .on_proc = on_faceproc_proc,
    .on_exit = on_faceproc_exit,
    .on_quit = on_faceproc_quit,
    // .proc_choose = faceproc_choose_proc,
};


int facesrv_start(struct proc_config *cfg)
{
    if (facesrv_core_init(cfg) < 0) {
        loge("facesrv_core_init failed. quit...\n");
        return -1;
    }

    proc_init(cfg, &cb);
    return 0;
}


int facesrv_stop()
{
    proc_quit();
    facesrv_core_uninit();
    return 0;
}


