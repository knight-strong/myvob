#ifndef __mysort_h_
#define __mysort_h_

#define LIST_POISON1 ((list_head *)-1)
#define LIST_POISON2 ((list_head *)-2)

#define LIST_HEAD_INIT(name) { &(name), &(name) }

struct list_head {
    struct list_head *next, *prev;
};

/*
 * 比较函数，决定两个节点的顺序
 * return： 0 or not 0
 *
 * */
typedef int compare_func(struct list_head *, struct list_head *);

/*
 * 插入节点并排序，调用f()决定节点的位置
 *
 * */
void list_insert_sort(struct list_head *newnode, struct list_head *head, compare_func f);


/*
 * 节点(安全)遍历
 *
 * */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
            pos = n, n = pos->next)


static inline void __list_add(struct list_head *newnode,
        struct list_head *prev,
        struct list_head *next)
{
    next->prev = newnode;
    newnode->next = next;
    newnode->prev = prev;
    prev->next = newnode;
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void __list_del_entry(struct list_head *entry)
{
        __list_del(entry->prev, entry->next);
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = LIST_POISON1;
    entry->prev = LIST_POISON2;
}


static inline void list_add_tail(struct list_head *newnode, struct list_head *head)
{
    __list_add(newnode, head->prev, head);
}

static inline void list_add(struct list_head *newnode, struct list_head *head)
{
        __list_add(newnode, head, head->next);
}

static inline void list_move(struct list_head *list, struct list_head *head)
{
    __list_del_entry(list);
    list_add(list, head);
}

#endif // end __mysort_h_

