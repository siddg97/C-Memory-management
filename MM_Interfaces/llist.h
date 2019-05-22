#ifndef __LLIST_H__
#define __LLIST_H__

struct LList{
    void* mem;
    struct LList *next;
};


struct LList* Init_node(void *p,struct LList* next);
void insertHead(struct LList **head, void *ptr);
void insertTail (struct LList **headRef, void* ptr);
void deleteNode (struct LList **head, void* ptr);
void destroyList(struct LList **head);
void List_sort(struct LList **head);
void List_sort_size(struct LList **head);
void printLL(struct LList **head);

#endif