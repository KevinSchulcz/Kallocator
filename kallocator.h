/*
 *   Kallocator - A simple memory allocator for Unix-like operating systems
 *
 *   Author: Kevin Schulcz
 *   Last Modified: Oct. 23rd, 2025
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


// ***** Typedefs *****

typedef unsigned char PAD[16];

// Header for each segment of allocared memory
//   Used in handling deallocation and reallocation.
//   Unioned to align headers to 16bytes.
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


// ***** Function Prototypes *****

header *find_free_segment(size_t size);
void *kmalloc(size_t size);
void kfree(void *segment_start);
void *kcalloc(size_t num_of_elements, size_t size_of_element);
void *krealloc(void *segment_start, size_t size);