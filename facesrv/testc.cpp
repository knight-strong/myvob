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



struct proc_config glb_cfg = {
    .proc_numb  =   4,
    .ef_size    =   -1,
};

static const char *filename(const char *f)
{
    const char *p = f + strlen(f);
    while (*p != '/' && p != f) p--;
    if (p == f)
        return p;
    return p + 1;
}

int main(int argc, char **argv)
{
    const char *f1 = "/home/hehj/workspace/facesrv/face/bin/photos/1-1.jpg";
    const char *f2 = "/home/hehj/workspace/facesrv/face/bin/photos/1-2.jpg";

    const char *f3 = "/home/hehj/workspace/facesrv/face/bin/photos/2-1.jpg";
    const char *f4 = "/home/hehj/workspace/facesrv/face/bin/photos/2-2.jpg";

    const char *FILE1 = "/home/hehj/workspace/facesrv/face/bin/photos/feature.idx";
    const char *FILE2 = "/home/hehj/workspace/facesrv/face/bin/photos/feature.idx";

    const char *fileidx_std = "/home/hehj/workspace/facesrv/face/bin/mydb/feature.idx";
    const char *fileidx_samples = "/home/hehj/workspace/facesrv/face/bin/samples/feature.idx";

    float score = 0;

    if (argc > 1)
        glb_cfg.proc_numb = atoi(argv[1]);

    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    logi("proc numb: %d\n", glb_cfg.proc_numb);

    facesrv_start(&glb_cfg);

    TICK_BEGIN();

    face_client c1;
    face_client c2;

#if 0
    feature_storage l(NULL);
    l.load(fileidx_samples);
    // l.dump();
#endif

#if 0
    if (c1.face_compare(f1, f2, &score) == 0) {
        logi("1 v 2 score: %f\n", score);
    }
    else {
        loge("compare failed\n");
    }

    if (c2.face_compare(f3, f4, &score) == 0) {
        logi("3 v 4 score: %f\n", score);
    }
    else {
        loge("compare failed\n");
    }
#endif

#if 0
    // c2.face_feature_gen("/home/hehj/workspace/facesrv/face/bin/mydb");
    logi("gen featurs ...\n");
    c2.face_feature_gen("/home/hehj/workspace/facesrv/face/bin/samples");
    TICK_PRINT("gen features done");
#endif

#if 1
    struct feature_compare_result *rs = c1.face_features_compare(fileidx_std, fileidx_samples);
    TICK_PRINT("compare done.");
    logi("dump NvN result:------------------------------\n");
#if 0
    logi("ignored ...\n");
#else
    if (rs) {
        list_head *pl;
        int i = 0;
        list_for_each(pl, &rs->l) {
            struct feature_compare_result *p = (struct feature_compare_result *)pl;
#if 0
            logi("compare rs:[%d] %f | %s | %s\n",
                    i++, p->score, p->f1, p->f2);
#else
            logi("%s|%s|%f\n", p->f1, p->f2, p->score);
            // logi("%s|%s|%f\n", filename(p->f1), filename(p->f2), p->score);
#endif
        }
    }
    logi("-------<result end>--------------------\n");
#endif
#endif

#if 0
    feature_lookup_result * rs1 = c1.face_lookup(f3, FILE1);
    // feature_lookup_result * rs1 = c1.face_lookup(f3, fileidx_std);
    logi("dump lookup result: [%s]--------------------------\n", f3);
    if (rs1) {
        list_head *pl;
        int i = 0;
        list_for_each(pl, &rs1->l) {
            struct feature_lookup_result *p = (struct feature_lookup_result *)pl;
            logi("compare rs:[%d] %f | %s\n", i++, p->score, p->f);
        }
    }
    logi("-------<result end>--------------------\n");
#endif

    TICK_PRINT("done");
    facesrv_stop();

    return 0;
}

