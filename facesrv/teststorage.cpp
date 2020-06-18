#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "simplelog.h"
#include "storage.h"

#define FILE_IDX "/home/hehj/workspace/facesrv/feature.idx"

int main(int argc, char **argv)
{
    /*
    feature_storage s(NULL);

    uint8_t *feature;
    struct feature f1, f2, f3;

    feature = new uint8_t[2560];
    memset(feature, 0x64, 2560);

    f1.feature = f2.feature = f3.feature = feature;
    // printf("%s", f1.feature);

    f1.size = f2.size = f3.size = 2560;
    f1.file = "file1",
    f2.file = "file2",
    f3.file = "file3",
    f1.desc = f2.desc = f3.desc = NULL;

    s.append_rec(&f1);
    s.append_rec(&f3);
    s.append_rec(&f2);

    s.dump();

    s.save(FILE_IDX);
*/
    logi("dump idx %s ....\n", argv[1]);
    feature_storage l(NULL);
    l.load(argv[1]);
    // l.load(FILE_IDX);
    l.dump();

    return 0;
}
