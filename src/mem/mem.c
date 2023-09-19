#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

typedef char ALIGN[16];
//alias for char array of size 16

union header{
    struct header_t{
        size_t size;
        unsigned is_free;
        struct header_t *next;
    } s;
    ALIGN stub;
};
typedef union header header_t;
//alias for union header we made

header_t *head=NULL, *tail=NULL;
//keeps track of our memory list

pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size){
    header_t *curr  = head;
    while(curr){
        if(curr->s.is_free && curr->s.size >= size){
            return curr;
        }
        curr = curr->s.next;
    }
    return NULL;
}

void free(void *block){
    header_t *header, *tmp;
    void *programbreak;

    if(!block){
        return;
    }
    pthread_mutex_lock(&global_malloc_lock);
    header = (header_t*)block - 1;
    //this gets the header of the block we want 
    //to free by casting block to a header pointer
    //type and move it behind by 1 unit

    programbreak = sbrk(0);
    //gives the current value of program break
    if((char*)block + header->s.size == programbreak){
        //to check if block to be freed is at the end 
        //of the heap, we must find the end of the current
        //block.
        //the end can be computed as
        //(char*)block + header->s.size 
        if(head == tail){
            head = tail = NULL;
        }
        else{
            tmp = head;
            while(tmp){
                if(tmp->s.next == tail){
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        sbrk(0 - sizeof(header_t) - header->s.size);
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

void *malloc(size_t size){
    size_t total_size;
    //size_t is an unsigned type that 
    //means it cant represent values <= 0
    void *block;
    header_t *header;
    if(!size){
        return NULL;
    }

    pthread_mutex_lock(&global_malloc_lock);
    header=get_free_block(size);
    if(header){
        header->s.is_free = 0;
        //marks block as not free
        pthread_mutex_unlock(&global_malloc_lock);
        return (void*)(header+1);
    }
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);
    //requests the os to increment the program break
    if(block == (void*)-1){
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }
    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if(!head){
        head = header;
    }
    if(tail){
        tail->s.next = header;
    }
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return (void*)(header+1);
}

/*
The calloc(num, nsize) function allocates memory 
for an array of num elements of nsize bytes each 
and returns a pointer to the allocated memory. 
Additionally, the memory is all set to zeroes. */
void *calloc(size_t num, size_t nsize){
    size_t size;
    void *block;
    if(!num || !nsize){
        return NULL;
    }
    size = num * nsize;
    /*check mul overflow*/
    if(nsize != size/num){
        return NULL;
    }

    block = malloc(size);
    if(!block){
        return NULL;
    }
    memset(block, 0, size);
    return block;
}
/*
realloc() changes the size of 
the given memory block to the 
size given.*/
void *realloc(void *block, size_t size){
    header_t *header;
    void *ret;
    if(!block || !size){
        return malloc(size);
    }
    header = (header_t*)block-1;
    if(header->s.size >= size){
        return block;
    }
    ret = malloc(size);
    if(ret){
        memcpy(ret, block, header->s.size);
        free(block);
    }
    return ret;
}

void print_mem_list(){
    header_t *curr = head;
    printf("head = %p, tail = %p\n", 
        (void*)head, (void*)tail);
    while(curr){
        printf("addr = %p, size = %zu, is_free = %u,"
            " next = %p\n", (void*)curr, curr->s.size,
            curr->s.is_free, (void*)curr->s.next);
        curr = curr->s.next;
    }
}