#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "sort.h"
#include "simplelog.h"
#include "storage.h"

feature_storage::feature_storage(const char *filename) {
    f_head.next = f_head.prev = &f_head;
    num = 0;
    if (filename)
        file = strdup(filename);
    else
        file = NULL;
    count = 0;
}

feature_storage::~feature_storage() {
    // todo
}

int feature_storage::append_rec(struct feature *f) {
    count++;
    list_add_tail((list_head *)f, (list_head *)&f_head);
    return 0;
}

int feature_storage::remove_by_file(const char *file) {
    return 0;
}

int feature_storage::remove_by_desc(const char *desc) {
    return 0;
}

struct feature * feature_storage::lookup_by_file(const char *file) {
    return NULL;
}

struct feature * feature_storage::lookup_by_desc(const char *file) {
    return NULL;
}

#define CHECK_RDWR_RET() if (r <= 0) goto error_exit;

static inline int write_string(int fd, const char *s)
{
    // format: len: int16_t, string[with null]
    int r;
    int16_t l;
    if (s)
        l = strlen(s);
    else
        l = 0;
    
    r = write(fd, &l, sizeof(l));
    CHECK_RDWR_RET();

    if (l > 0) {
        r = write(fd, s, l + 1);
        CHECK_RDWR_RET();
    }

    r = 0;
error_exit:
    return r;
}

static inline int write_int16(int fd, int16_t n)
{
    return write(fd, &n, sizeof(int16_t)) == sizeof(int16_t) ? 0 : -1;
}

static inline int write_data(int fd, void *d, int16_t l)
{
    int r = write(fd, &l, sizeof(int16_t));
    if (r != sizeof(int16_t))
            return -1;
    return write(fd, d, l) == l ? 0 : -1;
}


static inline int read_string(int fd, char **s)
{
    // format: len: int16_t, string[with null]
    int r;
    int16_t l;
    char *d = NULL;
    *s = NULL;
    
    r = read(fd, &l, sizeof(l));
    if (r != sizeof(l)) {
        r = -1;
        goto error_exit;
    }
    if (l > 0) {
        d = new char[l + 1];
        r = read(fd, d, l + 1);
        if (r != l + 1) {
            r = -1;
            goto error_exit;
        }
        *s = d;
    }

    r = 0;
error_exit:
    if (r < 0 && d) {
        delete[] d;
    }

    return r;
}


static inline int read_int16(int fd, int16_t *n)
{
    int r = read(fd, n, sizeof(int16_t));
    return r == sizeof(int16_t) ? 0 : -1;
}

static int read_data(int fd, void **data, int16_t *len)
{
    int r;
    int32_t l;
    char *d = NULL;
    *data = NULL;
    *len = 0;
    r = read(fd, &l, sizeof(int16_t));
    CHECK_RDWR_RET();

    if (l > 0) {
        d = new char[l];
        r = read(fd, d, l);
        if (r == l) {
            *data = d;
            *len = l;
        }
        else {
            r = -1;
        }
    }
    r = 0;
error_exit:
    if (r < 0 && d) {
        delete[] d;
    }

    return r;
}

static const char *get_filename(const char *f) 
{
    const char *p = f + strlen(f);
    while (*p != '/') p--;
    return p + 1;
}

int feature_storage::save(const char *file)
{
    int r = -1;
    int fd;
    list_head *p;
    int n = 0;
    // file head
    uint16_t head[8] = {
        0xcdcd, // file flag
        0x0001, // version
        0,      // rec number, low
        0,      // rec number, hi
        0, 0, 0, 0
    };

    fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        loge("open %s failed. %s\n", file, strerror(errno));
        goto error_exit;
    }

    r = write(fd, head, sizeof(head));
    if (r != sizeof(head)) {
        loge("write %s failed. %s\n", file, strerror(errno));
        r = -1;
        goto error_exit;
    }

    list_for_each(p, &f_head) {
        struct feature *f = (struct feature *)p;
        r = write_string(fd, get_filename(f->file)); 
        if (r) {
            loge("write file failed. %s. %s\n", strerror(errno), f->file);
            break;
        }
        r = write_string(fd, f->desc); 
        if (r) {
            loge("write desc failed. %s\n", strerror(errno));
            break;
        }
        r = write_int16(fd, f->size); 
        if (r) {
            loge("write size failed. %s\n", strerror(errno));
            break;
        }
        r = write_data(fd, f->data, f->size); 
        if (r) {
            loge("write data failed. %s. %p\n", strerror(errno), f->data);
            break;
        }
        n++;
    }

    logi("### %d records saved ###\n", n);
    r = 0;
error_exit:
    if (fd >= 0)
        close(fd);
    return r;
}

struct feature * load_one_feature(int fd)
{
    int r;
    struct feature *f = new struct feature;
    if (!f) {
        loge("out of memory !!\n");
        return NULL;
    }
    memset(f, 0, sizeof(*f));

    r = read_string(fd, &f->file);
    if (r) goto error_exit;

    r = read_string(fd, &f->desc);
    if (r) goto error_exit;

    r = read_int16(fd, &f->size);
    if (r) goto error_exit;

    r = read_data(fd, (void **)&f->data, &f->size);
    if (r) goto error_exit;

    r = 0;
error_exit:
    // loge("### file:%s, ds:%d, dptr: %p \n", f->file, f->size, f->data);
    if (r) {
        if (f->file)
            delete f->file;
        if (f->desc)
            delete f->desc;
        delete f;
        return NULL;
    } 
    return f;
}

int feature_storage::load(const char *file)
{
    int r = -1;
    int fd;
    // file head
    uint16_t head[8] = {
        0xcdcd, // file flag
        0x0001, // version
        0,      // rec number, low
        0,      // rec number, hi
        0, 0, 0, 0
    };
    int id = 0;

    fd = open(file,  O_RDONLY);
    if (fd < 0) {
        loge("open %s failed. %s\n", file, strerror(errno));
        goto error_exit;
    }

    r = read(fd, head, sizeof(head));
    if (r != sizeof(head)) {
        loge("read %s failed. %s\n", file, strerror(errno));
        r = -1;
        goto error_exit;
    }
    logi("file flag 0x%04x, ver:0x%04x\n", head[0], head[1]);

    do {
        struct feature *f = load_one_feature(fd);
        if (f) {
            f->id = id++;
            append_rec(f);
        }
        else {
            break;
        }
    } while(1);

    logi("load [%d] records\n", id);
    r = 0;
error_exit:
    if (fd >= 0)
        close(fd);
    return r;
}

void feature_storage::cleanup()
{
    list_head *p, *n;
    list_for_each_safe(p, n, &f_head) {
        struct feature *f = (struct feature *)p;
        // logi("delete %s\n", f->file);
        if (f->file)
            delete f->file;
        if (f->desc)
            delete f->desc;
        if (f->data)
            delete f->data;
        delete f;
    }
}

void feature_storage::dump()
{
    list_head *p;
    list_for_each(p, &f_head) {
        struct feature *f = (struct feature *)p;
        uint32_t h = (f->size ? *((uint32_t *)f->data) : 0);
        logi("rec: [%d] %s, %s, size=%d, f:%08x\n", 
                f->id,
                f->file,
                f->desc,
                f->size,
                h 
            );
    }
}


