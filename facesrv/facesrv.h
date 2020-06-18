#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef __facesrv_h_
#define __facesrv_h_

typedef  uint8_t BYTE;

enum {
    REQ_CMD_FACE_COMPARE    = 0x1001,
    REQ_CMD_FEATURE_GEN     = 0x1002,
    REQ_CMD_FEATURE_CMP     = 0x1003,
};

class proc_task_data;

struct proc_request {
    void (*on_finish) (void *req, proc_task_data *data);
    void *object;
};

class proc_task_data {
    private:
        int req_cmd;

    public:
        int serno;
        struct proc_request *req;
        void *custom;
        int result;

    public:
        proc_task_data() {
            req = NULL;
            custom = NULL;
            result = 0;
        }

        virtual ~proc_task_data() { }
        void set_req_cmd(int v) { req_cmd = v; }
        int get_req_cmd() { return req_cmd; }
};

#define MAX_FILENAME    512

class proc_task_data_facecomp : public proc_task_data {
public:
        char files[2][MAX_FILENAME];
        float score;

    public:
        proc_task_data_facecomp() {
            set_req_cmd(REQ_CMD_FACE_COMPARE);
            memset(files, 0, sizeof(files));
        }

        virtual ~proc_task_data_facecomp() { }

        void setFiles(const char *file0, const char *file1) {
            strncpy(files[0], file0, MAX_FILENAME);
            strncpy(files[1], file1, MAX_FILENAME);
            files[0][MAX_FILENAME - 1] = 0;
            files[1][MAX_FILENAME - 1] = 0;
        }
};

class proc_task_data_feature_gen : public proc_task_data {

    public:
        char file[MAX_FILENAME];
        BYTE *feature;

    public:
        proc_task_data_feature_gen() {
            feature = NULL;
            memset(file, 0, sizeof(file));
            set_req_cmd(REQ_CMD_FEATURE_GEN);
        }

        virtual ~proc_task_data_feature_gen() {
            if (feature)
                delete feature;
        }

        void set_file(const char *infile) {
            strncpy(file, infile, MAX_FILENAME);
            file[MAX_FILENAME - 1] = 0;
        }
};

struct feature;

class proc_task_data_feature_cmp : public proc_task_data
{
    public:
        struct feature *features[2];
        float score;

    public:
        proc_task_data_feature_cmp(struct feature *f0, struct feature *f1) {
            features[0] = f0;
            features[1] = f1;
            set_req_cmd(REQ_CMD_FEATURE_CMP);
        }

        virtual ~proc_task_data_feature_cmp() {
        }

};

int facesrv_core_init(struct proc_config* cfg);

void facesrv_core_uninit();

int do_face_compare(proc_task_data_facecomp *td, int ch);
int do_face_feature_gen(proc_task_data_feature_gen *td, int ch);
void do_face_feature_cmp(proc_task_data_feature_cmp *t, int ch);

int facesrv_start(struct proc_config *cfg);
int facesrv_stop();

#endif

