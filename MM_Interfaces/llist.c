#include <stdio.h>
#include <stdlib.h>
#include "llist.h"
#include "myalloc.h"

// HELPER FUNCTIONS
//================================================================================
void swap(struct LList *a, struct LList *b){
        void* swap = a->mem;
        a->mem = b->mem;
        b->mem = swap;
}

//================================================================================



// LLIST API FUNCTIONS
//================================================================================
struct LList* Init_node( void *p, struct LList* next){
	struct LList *h = (struct LList*) malloc(sizeof(struct LList));
	h->mem = p;
	h->next = next;
	return h;
}

void insertHead(struct LList **head, void *ptr){
	struct LList *n;
	n = Init_node(ptr, *head);
	*head = n;
}

void insertTail (struct LList **headRef, void *ptr){
	struct LList* curr = *headRef;
	if(curr == NULL){
		insertHead(headRef, ptr);
	}
	else if(curr->next == NULL){
		struct LList *n = Init_node(ptr,NULL);
		curr->next = n;
	}
	else{
		while(curr->next != NULL){
			curr = curr->next;
		}
		insertHead(&(curr->next),ptr);
	}
}


// Delete node which has ptr in it node->mem field
void deleteNode (struct LList **head, void* ptr){
	//Empty linked list: Do nothing
	if(*head == NULL || ptr == NULL){
		return;
	}
	// store head
	struct LList* temp = *head;
	// If ptr is in *head
	if(ptr == temp->mem){
		*head = temp->next;  // new head
		free(temp);		// delete node
		return;
	}
	// node to be delted is in middle of list or the last node now
	while(temp->next->mem != ptr){
		temp = temp->next;
	}	// temp points to node right before node with ptr
	struct LList *nextOfnode = temp->next->next;
	free(temp->next);
	temp->next = nextOfnode;
}

void destroyList(struct LList **head){
	struct LList* temp = *head;
	if(temp == NULL){
		return;
	}
	else{
		void* t;
		while(temp != NULL){
			t = temp->mem;
			deleteNode(head,t);
			temp = temp->next;
		}
	}
}

// sort by ascending order of addresses
void List_sort (struct LList **head){
    int swapped; 
    struct LList *ptr1; 
    struct LList *lptr = NULL;
    struct LList *start = *head; 
  
    /* Checking for empty list */
    if (start == NULL) 
        return; 
  
    do
    { 
        swapped = 0; 
        ptr1 = start; 
  
        while (ptr1->next != lptr) 
        { 
            if (*(long *)(ptr1->mem) > *(long *)(ptr1->next->mem)) 
            {  
                swap(ptr1, ptr1->next); 
                swapped = 1; 
            } 
            ptr1 = ptr1->next; 
        } 
        lptr = ptr1; 
    } 
    while (swapped); 
}

// sort by ascending order of size[ptr-8]
void List_sort_size(struct LList **head){
	int swapped; 
    struct LList *ptr1; 
    struct LList *lptr = NULL;
    struct LList *start = *head; 
  
    /* Checking for empty list */
    if (start == NULL) 
        return; 
  
    do
    { 
        swapped = 0; 
        ptr1 = start; 
  
        while (ptr1->next != lptr) 
        { 
            if (*(long *)((ptr1->mem)-8) > *(long *)((ptr1->next->mem)-8)) 
            {  
                swap(ptr1, ptr1->next); 
                swapped = 1; 
            } 
            ptr1 = ptr1->next; 
        } 
        lptr = ptr1; 
    } 
    while (swapped); 
}

void printLL(struct LList **head){
	struct LList *h = *head;
	if(h == NULL){
		printf("Empty List\n");
		return;
	}
	int i = 1;
	while(h != NULL){
		if( h->mem == (myalloc.endPtr + 1)){
			printf("addr: %p size: 0\n",h->mem);
		}
		else{
			printf("Node[%d]: mem: %p size: %ld\n",i,h->mem, *(long *)((h->mem)-8));
		}
		h = h->next;
		i++;
	}
}

//======================================================================================