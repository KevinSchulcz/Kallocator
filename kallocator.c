#include <stddef.h>
#include <unistd.h>
#include <pthread.h>

// ***** Typedefs *****


typedef unsigned char PAD[16];

// Header for each segment of allocared memory
//  Used in handling deallocation and reallocation
//  Unioned to align headers to 16bytes
typedef union header {
    struct {
        size_t size;
        unsigned int is_free;
        union header *next;
    } h;
    PAD pad;
} header;


// ***** Global Vars *****


header *head;
header *tail;
pthread_mutex_t alloc_lock;


// ***** Functions *****

// Helper Function for kmalloc()
//   Finds a previously freed segment of memory
//   that is at least size in length and returns
//   a pointer to its header. If none exists, 
//   returns NULL.
header *find_free_segment(size_t size)
{
    header *curr = head;
    while (curr) {
        if (curr->h.is_free && (curr->h.size >= size)) return curr;
        curr = curr->h.next;
    }
    return NULL;
}

// Implementation of malloc()
//   Allocates a segment of memory of length at 
//   least size and returns a pointer to the start
//   of that segment. Returns NULL if unable to 
//   allocate a segment of length size.
void *kmalloc(size_t size) 
{
    void *segment_start = NULL;
    header *header = NULL;
    size_t total_size;

    if (!size) return NULL;

    pthread_mutex_lock(&alloc_lock);

    // Free existing segment available
    header = find_free_segment(size);
    if (header) {
        header->h.is_free = 0;
        pthread_mutex_unlock(&alloc_lock);
        return (void *)(header + 1);
    }

    // Need to alloc new segment
    total_size = sizeof(header) + size;
    segment_start = sbrk(total_size);
    if(segment_start == (void*)-1) {
        pthread_mutex_unlock(&alloc_lock);
        return NULL;                            // sbrk will return -1 on failure
    }
    header = segment_start;
    header->h.size = size;
    header->h.is_free = 0;
    header->h.next = NULL;
    if (!head) head = header;
    if (tail) tail->h.next = header;
    tail = header;
    pthread_mutex_unlock(&alloc_lock);

    return (void *)(header + 1);
}

