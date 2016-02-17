#ifndef _LISTS_INCLUDED
#define _LISTS_INCLUDED
#include "common.h"

typedef struct list_head_s list_head_t;
struct list_head_s {
	struct list_head *next, *prev;
};


static inline void INIT_LIST_HEAD(list_head_t *list) {
	list->next = list;
	list->prev = list;
}

static inline void __list_add(list_head_t *new,
			      list_head_t *prev, list_head_t *next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(list_head_t *new, list_head_t *head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(list_head_t *new, list_head_t *head)
{
	__list_add(new, head->prev, head);
}

static inline void __list_del(list_head_t * prev, list_head_t * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(list_head_t *entry) {
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

static inline int list_empty(const list_head_t *head) {
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)
        	
#define list_for_each_entry(pos, head, member)				\
            for (pos = list_entry((head)->next, typeof(*pos), member);  \
                 &pos->member != (head);    \
                 pos = list_entry(pos->member.next, typeof(*pos), member))

#endif
