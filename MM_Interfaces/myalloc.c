#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "myalloc.h"
#include "llist.h"
#include <stdbool.h>

// lock for synchronization
pthread_mutex_t lock;


// HELPER FUNCTIONS:
//========================================================================================================
// gets size in bytes of the chunk of memory pointed by the list node
long getSize(struct LList* node){
    void* temp = (node->mem)-8;
    return *(long *)temp;
}    

// sets the header of loaction pointed by node->mem to value pointed by s
void setSize(struct LList* node, long *s){
    void *t = (node->mem)-8;
    memcpy(t,&s,sizeof(long));
}

// HELPER FUNCTIONS: for printStatistics() 
long aSize(){ // return allocated size
    pthread_mutex_lock(&lock);
    struct LList* A = myalloc.Alloc;
    long s = 0;
    long hold;
    while(A != NULL){
        memcpy(&hold,A->mem-8,sizeof(long));
        s += hold;
        A = A->next;
    }
    pthread_mutex_unlock(&lock);
    return s;
}

int aCount(){ // return # allocated chunks
    pthread_mutex_lock(&lock);
    struct LList* A = myalloc.Alloc;
    int c = 0;
    while(A != NULL){
        c++;
        A = A->next;
    }
    pthread_mutex_unlock(&lock);
    return c;
}

int fCount(){ // return # free chunks
    pthread_mutex_lock(&lock);
    struct LList* F = myalloc.Free;
    if(F->mem == (myalloc.endPtr + 1)){
        pthread_mutex_unlock(&lock);
        return 0;
    }
    else{
        int c = 0;
        while(F != NULL){
            c++;
            F = F->next;
        }
        pthread_mutex_unlock(&lock);
        return c;
    }
}

int minfSize(){ // return min free size chunk
    pthread_mutex_lock(&lock);
    struct LList* F = myalloc.Free;
    if(F->mem == (myalloc.endPtr + 1)){
        pthread_mutex_unlock(&lock);
        return 0;
    }
    long min = *(long*)(F->mem-8);
    long hold;
    F = F->next;
    while(F != NULL){
        hold = *(long*)(F->mem-8);
        if(hold < min){
            min = hold;
        }
        F = F->next;
    }
    pthread_mutex_unlock(&lock);
    return min;
}

int maxfSize(){ // return max free size chunk
    pthread_mutex_lock(&lock);
     struct LList* F = myalloc.Free;
    if(F->mem == (myalloc.endPtr + 1)){
        pthread_mutex_unlock(&lock);
        return 0;
    }
    long max = *(long*)(F->mem-8);
    long hold;
    F = F->next;
    while(F != NULL){
        hold = *(long*)(F->mem-8);
        if(hold > max){
            max = hold;
        }
        F = F->next;
    }
    pthread_mutex_unlock(&lock);
    return max;
}

// HELPER FUNCTION: available_memory() and printstats()
long fSize(){ // return free size
    pthread_mutex_lock(&lock);
    struct LList* F = myalloc.Free;
    if(F->mem == (myalloc.endPtr + 1)){
        pthread_mutex_unlock(&lock);
        return 0;
    }
    else{
        long s = 0;
        long hold;
        while(F != NULL){
            memcpy(&hold,F->mem-8,sizeof(long));
            s += hold;
            F = F->next;
        }
        pthread_mutex_unlock(&lock);
        return s;
    }
}

// to merge contigious memory after compaction
void compactionMerge(struct LList* a, long asize, struct LList* b){
    if(a == NULL || b == NULL){
        return;
    }
    else{
        if(a->mem + asize == b->mem){ // contigious
            long n = asize + getSize(b);
            memcpy(a->mem-8,&n,sizeof(long));
            a->next = b->next;
            deleteNode(&(myalloc.Free),b->mem); // delete disconnected node
        }
    }
}

void fix(struct LList* a){
    void* t = a->mem - 8;
    void* t1 = a->next->mem - 8;
    long sa,sb,res;
    memcpy(&sa,t,sizeof(long));     // size of a
    memcpy(&sb,t1,sizeof(long));    // size of a->next
    res = sa+sb;                    // combined size of a and a->next
    memcpy(t,&res,sizeof(long));
    a->next = a->next->next;        // update a->next
}

bool isContigious(struct LList* a, struct LList* b){
    bool retval = false;
    if(b == NULL || a == NULL){
        return retval;
    }
    if(a->mem +getSize(a) == b->mem){
        retval = true;
    }
    return retval;
}

void mergeMem(){
    List_sort(&(myalloc.Free)); // sort in ascending order of mem addresses
    struct LList* current = myalloc.Free;
    while(current && current->next){
        if(isContigious(current,current->next)){
            fix(current);
        }
        current = current->next;        
    }
}




//============================================================================================================================



// API FUNCTIONS:
//============================================================================================================================
void initialize_allocator(int _size, enum allocation_algorithm _aalgorithm) {
    assert(_size > 0);
    myalloc.aalgorithm = _aalgorithm;
    myalloc.size = _size;                                                                           
    myalloc.memory = malloc((size_t)myalloc.size);   // Allocate memory chunk of size _size         
    void *currMem = myalloc.memory;                                                                 
    long s = _size;                                                                                
    memset(myalloc.memory,'0',myalloc.size);        // Init. memory to 0                                          
    memcpy(currMem,&s,sizeof(long)); // set header for empty memory;                                
    currMem += 8;                   // First address in memory that is free                         
    // Init. LL for allocated and free memory                                                       
    // struct LList* A = Init_node(NULL,NULL);     // Initialize alloc list
    struct LList* F = Init_node(currMem,NULL);  // Initialize free list
    // myalloc.Alloc = A;
    myalloc.Free = F;
    myalloc.startPtr = myalloc.memory;
    myalloc.endPtr = myalloc.memory + _size-1;
}                                                                                                                                                                             

void destroy_allocator() {
    pthread_mutex_lock(&lock);
    free(myalloc.memory);
    destroyList(&(myalloc.Free));
    destroyList(&(myalloc.Alloc));
    pthread_mutex_unlock(&lock);
    // free other dynamic allocated memory to avoid memory leak
}

void* allocate(int _size) {
    pthread_mutex_lock(&lock);          // Synchronisation lock [MUTUAL EXECUTION]
    void* ptr = NULL;                   // pointer to be returned
    void* hptr;                         // pointer for the header which has size
    void* temp;                         // temp pointer for misc operations
    long s = _size+8;                   // Actual size to be allocated and written to header
    long size = _size;                  // size to be used for comparision 
    long fsize;                         // size in header [will be retrieved] in free list
    bool flag = false;
    struct LList* tempF = myalloc.Free; // temp pointer to head of free list
    if( tempF->mem == (myalloc.endPtr + 1)){
            pthread_mutex_unlock(&lock);
            return NULL;
    }
    switch(myalloc.aalgorithm){
        case FIRST_FIT:
            // Look for the first free chunk which can acomodate the given size
            while(tempF != NULL){
                if(getSize(tempF) >= s){
                    flag = true;
                    break;
                }
                tempF = tempF->next;
            } // tempF points to first fit node in free list or else flag is flase!
            
            if(flag){ // if a fit was found
                ptr = tempF->mem;   // return the address free for allocation
                hptr = ptr-8;
                memcpy(&fsize, hptr,sizeof(long)); // fsize gets free size before allocation
                fsize -= s;         // new free size;
                if(fsize > 8){
                    memcpy(hptr,&s,sizeof(long));   // set header size for allocated block
                    insertTail(&(myalloc.Alloc),ptr);
                    temp = ptr + s;  // next free memory space
                    tempF->mem = temp;
                    temp -= 8; // free size header
                    memcpy(temp,&fsize,sizeof(long)); // copy new free size into new header
                }
                else{
                    s += fsize;
                    memcpy(hptr,&s,sizeof(long));   // set header size for allocated block
                    insertTail(&(myalloc.Alloc),ptr);
                    destroyList(&(myalloc.Free));
                    myalloc.Free = Init_node(myalloc.endPtr + 1,NULL);
                }
            }
            else{
                pthread_mutex_unlock(&lock);
                return NULL;
            }
            break;

        case BEST_FIT:
            List_sort_size(&(myalloc.Free)); // sort free list into ascending order of size
            // NOW: the first chunk with size greater than or equal to that requested is the best fit for allocation
            // OR IN OTHER WORDS: It will now follow the procedure for the first fit algorithm
            while(tempF != NULL){
                if(getSize(tempF) >= s){
                    flag = true;
                    break;
                }
                tempF = tempF->next;
            } // tempF points to first fit node in free list or else flag is flase!
            
            if(flag){ // if a fit was found
                ptr = tempF->mem;   // return the address free for allocation
                hptr = ptr-8;
                memcpy(&fsize, hptr,sizeof(long)); // fsize gets free size before allocation
                fsize -= s;         // new free size;
                if(fsize > 8){
                    memcpy(hptr,&s,sizeof(long));   // set header size for allocated block
                    insertTail(&(myalloc.Alloc),ptr);
                    temp = ptr + s;  // next free memory space
                    tempF->mem = temp;
                    temp -= 8; // free size header
                    memcpy(temp,&fsize,sizeof(long)); // copy new free size into new header
                }
                else{
                    s += fsize;
                    memcpy(hptr,&s,sizeof(long));   // set header size for allocated block
                    insertTail(&(myalloc.Alloc),ptr);
                    destroyList(&(myalloc.Free));
                    myalloc.Free = Init_node(myalloc.endPtr + 1,NULL);
                }
            }
            else{
                pthread_mutex_unlock(&lock);
                return NULL;
            }
            break;

        case WORST_FIT:
            List_sort_size(&(myalloc.Free));
            // NOW: the last node in the free list has the largest size hence if that node can allocate the requested size then that is the worst fit
            // OR IN OTHER WORDS: allocate at the last node in free list if possible
            while(tempF->next != NULL){
                tempF = tempF->next;
            } // tempF points to last node in free list which has largest size
            
            if(s <= getSize(tempF)){ // if requested size can be accomodated
                ptr = tempF->mem;   // return the address free for allocation
                hptr = ptr-8;
                memcpy(&fsize, hptr,sizeof(long)); // fsize gets free size before allocation
                fsize -= s;         // new free size;
                if(fsize > 8){
                    memcpy(hptr,&s,sizeof(long));   // set header size for allocated block
                    insertTail(&(myalloc.Alloc),ptr);
                    temp = ptr + s;  // next free memory space
                    tempF->mem = temp;
                    temp -= 8; // free size header
                    memcpy(temp,&fsize,sizeof(long)); // copy new free size into new header
                }
                else{
                    s += fsize;
                    memcpy(hptr,&s,sizeof(long));   // set header size for allocated block
                    insertTail(&(myalloc.Alloc),ptr);
                    destroyList(&(myalloc.Free));
                    myalloc.Free = Init_node(myalloc.endPtr + 1,NULL);
                }
            }
            else{
                pthread_mutex_unlock(&lock);
                return NULL;
            }
            break;
    }
    pthread_mutex_unlock(&lock);
    return ptr;
}


void deallocate(void* _ptr) {
    pthread_mutex_lock(&lock);
    if(_ptr == NULL){
        printf("NULL pointer recieved by deallocator()\n");
        pthread_mutex_unlock(&lock);
        return;
    }
    if((myalloc.Free)->mem == myalloc.endPtr + 1){
        myalloc.Free->mem = _ptr;
        deleteNode(&(myalloc.Alloc),_ptr); // delete the allocation metadata
        mergeMem();             // manage contigiousness of free list.
        pthread_mutex_unlock(&lock);
        return;    
    }
    else{
        insertTail(&(myalloc.Free),_ptr); // add to the free list
        deleteNode(&(myalloc.Alloc),_ptr); // delete the allocation metadata
        mergeMem();             // manage contigiousness of free list.
        pthread_mutex_unlock(&lock);
        return;
    }
    // Free allocated memory
    // Note: _ptr points to the user-visible memory. The size information is
    // stored at (char*)_ptr - 8.
}

int compact_allocation(void** _before, void** _after) {
    // ALgorithm: keep swapping allocated chunks (on the "right" or "below" free chunks) to the "left" or "up" and merge free sections on the "right", till no other such allocated chunk is found
    pthread_mutex_lock(&lock);
    int compacted_size = 0;
    int i = 0;
    struct LList* A = myalloc.Alloc;
    struct LList* F = myalloc.Free;
   
    if(A == NULL || F == NULL){ // no allocation or no free space
        pthread_mutex_unlock(&lock);
        return 0;
    }

    void* tFreeMem = F->mem;
    void* tAllocMem;

    while(A != NULL){   // find the allocated node which points to memory after the first free chunk
        tAllocMem = A->mem;
        if(*(long*)tAllocMem > *(long*)tFreeMem){
            break;
        }
        _before[i] = A->mem;
        _after[i] = A->mem;
        i++;
        A = A->next;
    }
    long sizeA,sizeF;
    while(A != NULL){
        _before[i] = A->mem;
        sizeA = getSize(A);     // size of allocated chunk
        sizeF = getSize(F);     // size of free chunk
        memcpy(F->mem,A->mem,(size_t)(sizeA - 8)); // transfer chunk to free space
        memcpy(F->mem-8,&sizeA,sizeof(long)); // update size for transferred chunk
        A->mem = F->mem;
        F->mem += sizeA;
        setSize(F,sizeF);
        _after[i] = A->mem;
        mergeMem();
        A = A->next;
        i++;
    }
    compacted_size = i;


    // compact allocated memory
    // update _before, _after and compacted_size
    pthread_mutex_unlock(&lock);
    return compacted_size;
}

int available_memory() {
    return fSize();
}

void print_statistics() {
    long allocated_size = 0;
    int allocated_chunks = 0;
    long free_size = 0;
    int free_chunks = 0;
    int smallest_free_chunk_size = myalloc.size;
    int largest_free_chunk_size = 0;

    // Calculate the statistics
    allocated_size = aSize();
    allocated_chunks = aCount();
    free_size = myalloc.size - allocated_size;
    free_chunks = fCount();
    smallest_free_chunk_size = minfSize();
    largest_free_chunk_size = maxfSize();

    printf("Allocated size = %d\n", allocated_size);
    printf("Allocated chunks = %d\n", allocated_chunks);
    printf("Free size = %d\n", free_size);
    printf("Free chunks = %d\n", free_chunks);
    printf("Largest free chunk size = %d\n", largest_free_chunk_size);
    printf("Smallest free chunk size = %d\n", smallest_free_chunk_size);
}



