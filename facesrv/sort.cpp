#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sort.h"

#define TEST    0
#define DEBUG   0
#if DEBUG
#define logd(...) printf(__VA_ARGS__)
#define logi(...) printf(__VA_ARGS__)
#else
#define logd(...)
#define logi(...) printf(__VA_ARGS__)
#endif
//-----------------------------------

void list_insert_sort(struct list_head *newnode, struct list_head *head, compare_func f)
{
    struct list_head *list;

    list_for_each(list, head) {
        if (f(newnode, list)) {
            list_add_tail(newnode, list);
            return;
        }
    }
    list_add(newnode, head->prev);
}

