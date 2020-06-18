#ifndef __storage_h_
#define __storage_h_

#include "sort.h"

struct feature {
    struct list_head l;
    char *file;
    char *desc;
    unsigned char *data;
    int16_t size;
    int id;     // not saved
};

class feature_storage {
    private:
        struct list_head f_head; 
        const char *file;
        int count;

    public:
        int num;
        int get_count() { return count; }
        const char *filename() { return file; }
        feature_storage(const char *file);
        virtual ~feature_storage();

        int append_rec(struct feature *f);

        int remove_by_file(const char *file);
        int remove_by_desc(const char *desc);

        struct feature * lookup_by_file(const char *file);
        struct feature * lookup_by_desc(const char *file);

        int save() { return save(file); }
        int save(const char *file);
        int load(const char *file);

        struct list_head * get_head() { return &f_head; }

        void cleanup();
        void dump();
};

#endif
