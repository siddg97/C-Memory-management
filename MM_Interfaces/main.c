#include <stdio.h>
#include "myalloc.h"
#include "llist.h"

void printChunk(){
	void *temp;
	for(int i=0; i < 100; i++){
    	temp = myalloc.memory + i;
    	printf("myalloc.memory[%d]-addr: %p\n",i+1/*,*(char*)temp*/,temp);
    }
}

void printUsedChunk(){
	struct LList* t = myalloc.Free;
	printf("Free memory holes:\n");
	while(t != NULL){
		if( t->mem == (myalloc.endPtr + 1)){
			printf("addr: %p size: 0\n",t->mem);
		}
		else{
			printf("addr: %p size: %ld\n",t->mem,*(long *)(t->mem - 8));
		}
		t = t->next;
	}
}

int main(int argc, char* argv[]) {
    initialize_allocator(100, FIRST_FIT);
    printf("Using first fit algorithm on memory size 100\n");
    
    //initialize_allocator(100, BEST_FIT);
    //printf("Using best fit algorithm on memory size 100\n");
    
    //initialize_allocator(100, WORST_FIT);
    //printf("Using worst fit algorithm on memory size 100\n");
    void *t = ((myalloc.Free)->mem)-8;
    printf("Size of free mem in initial Chunk allocated: %ld\n",*(long *)t);
    printf("\n\n");
    printChunk();
    printf("\n\n");
    int* p[50] = {NULL};
    for(int i=0; i<10; ++i) {
        p[i] = allocate(sizeof(int));
        if(p[i] == NULL) {
            printf("Allocation failed\n");
            continue;
        }
        *(p[i]) = i;
        printf("p[%d] = %p ; *p[%d] = %d\n", i, p[i], i, *(p[i]));
    }
    printf("\n");
    printUsedChunk();
    printf("\n");
    printf("\nFree List\n");
    printLL(&(myalloc.Free));
    printf("\nAllocated List\n");
    printLL(&(myalloc.Alloc));
    
    printf("\n\n");

    print_statistics();
	
	printf("\n\n");
    

    for (int i = 0; i < 4; ++i) {
        printf("Freeing p[%d]\n", i);
        deallocate(p[i]);
        p[i] = NULL;
    }


    printf("\n");
    printUsedChunk();
    printf("\n");
    printf("\nFree List\n");
    printLL(&(myalloc.Free));
    printf("\nAllocated List\n");
    printLL(&(myalloc.Alloc));
    printf("\n\n");
    printf("available_memory %d", available_memory());
    printf("\n\n");
    print_statistics();
    printf("\n\n");

    void* before[100] = {NULL};
    void* after[100] = {NULL};
    compact_allocation(before, after);

    printf("\n");
    printf("\nFree List\n");
    printLL(&(myalloc.Free));
    printf("\nAllocated List\n");
    printLL(&(myalloc.Alloc));
    printf("\n\n");

    print_statistics();

    // // You can assume that the destroy_allocator will always be the 
    // // last funciton call of main function to avoid memory leak 
    // // before exit

    destroy_allocator();

    return 0;
}
