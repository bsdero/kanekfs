#ifndef _LIST_H_
#define _LIST_H_


struct list_entry {
    struct list_entry *next;
    struct list_entry *prev;
};

typedef struct list_entry list_t;
typedef struct list_entry list_entry_t;


#define LIST(l) ((list_t *)(l))

#define INIT_LIST_HEAD(ptr) do {     \
        (ptr)->next = (ptr); \
        (ptr)->prev = (ptr); \
        } while (0)

#define LIST_HEAD_INIT(ptr) { &(ptr), &(ptr) }

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))


static inline int list_empty (list_t *list){
    return (list->next == list);
}

static inline void list_add (list_t *new_entry, list_t *head){
    new_entry->next  = head->next;
    new_entry->prev  = head;
    head->next->prev = new_entry;
    head->next       = new_entry;
}

static inline void list_add_tail (list_t *new_entry, list_t *head){
    new_entry->next  = head;
    new_entry->prev  = head->prev;
    head->prev->next = new_entry;
    head->prev       = new_entry;
}

static inline void list_del (list_t *entry){
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

static inline void list_reparent (list_t *list, list_t *new_entry){
    if (list_empty(list))
        return;

    new_entry->next = list->next;
    new_entry->prev = list->prev;
    new_entry->prev->next = new_entry;
    new_entry->next->prev = new_entry;
}


#endif

