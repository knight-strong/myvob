#include <semaphore.h>
#include <pthread.h>
#include "sort.h"
#include "facesrv.h"

#ifndef __client_h_
#define __client_h_

class feature_storage;

struct feature_compare_result {
    struct list_head l;
    float score;
    char f1[MAX_FILENAME];
    char f2[MAX_FILENAME];
};

struct feature_lookup_result {
    struct list_head l;
    float score;
    char f[MAX_FILENAME];
};

extern struct proc_config glb_cfg;

class face_client {
    private:
        int msqid;
        int state;

        // sem_t sem;
        // pthread_mutex_t mutex;

        struct proc_request request;

    protected:
        void on_compare_finish(proc_task_data *data);
        int peek_message(struct message *msg);
        int get_message(struct message *msg);

        // call by proc-task thread
        static void on_task_finish(void *req, proc_task_data *data);

        int parse_result(struct message *msg, float *score);
        int parse_result(struct message *msg, struct feature **f);
        int parse_result(struct message *msg, feature_compare_result *rs);
    
    public:
        face_client();
        virtual ~face_client();

        int face_compare(const char *f1, const char *f2, float *score);

        int face_feature_gen(const char *dir);

        struct feature_compare_result *face_features_compare(const char *idx1, const char *idx2);

        struct feature_lookup_result *face_lookup(const char *file, feature_storage *s);
        struct feature_lookup_result *face_lookup(const char *file, const char *idx);
        struct feature_lookup_result *face_lookup(const char *file);
};
#endif

