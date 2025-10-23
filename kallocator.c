#include "kallocator.h"

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
    header *hdr = NULL;
    size_t total_size;

    if (!size) return NULL;

    pthread_mutex_lock(&alloc_lock);

    // Free existing segment available
    hdr = find_free_segment(size);
    if (hdr) {
        hdr->h.is_free = 0;
        pthread_mutex_unlock(&alloc_lock);
        return (void *)(hdr + 1);
    }

    // Need to alloc new segment
    total_size = sizeof(hdr) + size;
    segment_start = sbrk(total_size);
    if(segment_start == (void*)-1) {
        pthread_mutex_unlock(&alloc_lock);
        return NULL;                            // sbrk will return -1 on failure
    }
    hdr = segment_start;
    hdr->h.size = size;
    hdr->h.is_free = 0;
    hdr->h.next = NULL;
    if (!head) head = hdr;
    if (tail) tail->h.next = hdr;
    tail = hdr;
    pthread_mutex_unlock(&alloc_lock);

    return (void *)(hdr + 1);
}

// Implementation of free()
//   Frees the segment of memory that starts at 
//   the address segment_start. If the segment 
//   is at the end of the heap, it fully releases 
//   the memory back to the OS. If the segment 
//   is not the tail segment, it will just set it 
//   as "free", to be used in a future alloc.
void kfree(void *segment_start)
{
    header *curr = head;
    header *hdr = NULL;
    void *brk;

    if (!segment_start) return;

    pthread_mutex_lock(&alloc_lock);
    hdr = (header *)(segment_start - 1);
    brk = sbrk(0);

    printf("Debug: brk - %p\n", brk);
    printf("Debug: seg - %p\n", ((char *)segment_start + hdr->h.size));

    // Segment is at the very end of the heap
    if (((char *)segment_start + hdr->h.size) == brk) {
        if (head == tail) {
            head = NULL;
            tail = NULL;
        }
        else {
            while (curr) {
                if (curr->h.next == tail) {
                    curr->h.next = NULL;
                    tail = curr;
                }
                
                curr = curr->h.next;
            }
        }

        sbrk(0 - (sizeof(header) + hdr->h.size));
        pthread_mutex_unlock(&alloc_lock);
        return;
    }

    // Segment is not the last allocated segment in the heap
    hdr->h.is_free = 1;
    pthread_mutex_unlock(&alloc_lock);

    return;
}

// Implementation of calloc()
//   Allocates a packed segment of memory of 
//   num_of_elements elements each of size 
//   size_of_element, and initalizes all bytes 
//   in the segment to 0. Returns a pointer to 
//   the start of the segment or returns NULL if 
//   allocation failed. Calls upon kmalloc().
void *kcalloc(size_t num_of_elements, size_t size_of_element)
{
    size_t size;
    void *segment_start = NULL;

    if (!num_of_elements || !size_of_element) return NULL;

    size = num_of_elements * size_of_element;
    if (size_of_element != (size / num_of_elements)) return NULL;   // Overflow after multiplication
    
    segment_start = kmalloc(size);
    if (!segment_start) return NULL;
    memset(segment_start, 0, size);

    return segment_start;
}

// Implementation of realloc()
//   Reallocate the given memory segment to be of 
//   length at least size, and return a pointer 
//   to the new segment. The pointer may point to 
//   the same or a new address. Returns NULL if 
//   failed to reallocate. If the segment pointer 
//   is moved to a new address, the entirety of 
//   the previous segment's memory content will 
//   be moved with it, and the old memory segment 
//   will be freed. Calls upon kmalloc() and 
//   kfree().
void *krealloc(void *segment_start, size_t size)
{
    header *hdr = NULL;
    void *new_start = NULL;

    if (!segment_start || !size) return NULL;

    // The segment is already large enough for the new size
    hdr = (header *)(segment_start - 1);
    if (hdr->h.size >= size) return segment_start;

    // Need a large segment to accommodate the new size
    new_start = kmalloc(size);
    if (new_start) {
        memcpy(new_start, segment_start, hdr->h.size);
        kfree(segment_start);
    }

    return new_start;
}