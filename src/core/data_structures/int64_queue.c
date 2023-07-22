#include "core/data_structures/int64_queue.h"

#include <assert.h>   // assert
#include <malloc.h>   // realloc, free
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdint.h>   // int64_t
#include <stdio.h>    // fprintf, stderr
#include <string.h>   // memcpy

static bool Expand(Int64Queue *queue) {
    int64_t new_capacity = queue->capacity == 0 ? 1 : queue->capacity * 2;
    int64_t *new_array =
        (int64_t *)realloc(queue->array, new_capacity * sizeof(int64_t));
    if (new_array == NULL) return false;
    memcpy(&new_array[queue->capacity], new_array,
           queue->front * sizeof(int64_t));
    queue->array = new_array;
    queue->capacity = new_capacity;
    return true;
}

// Function to initialize the queue
void Int64QueueInit(Int64Queue *queue) {
    queue->array = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->size = 0;
}

// Function to destroy the queue and free the allocated memory
void Int64QueueDestroy(Int64Queue *queue) {
    free(queue->array);
    queue->array = NULL;
    queue->capacity = 0;
    queue->front = 0;
    queue->size = 0;
}

// Function to check if the queue is empty
bool Int64QueueIsEmpty(const Int64Queue *queue) { return (queue->size == 0); }

// Function to check the size of the queue
int64_t Int64QueueSize(const Int64Queue *queue) { return queue->size; }

// Function to add an element to the queue
bool Int64QueuePush(Int64Queue *queue, int64_t element) {
    if (queue->size == queue->capacity) {
        if (!Expand(queue)) return false;
    }
    assert(queue->size < queue->capacity);
    int64_t back = (queue->front + queue->size) % queue->capacity;
    queue->array[back] = element;
    ++queue->size;
    return true;
}

// Function to remove an element from the queue
int64_t Int64QueuePop(Int64Queue *queue) {
    if (Int64QueueIsEmpty(queue)) {
        fprintf(stderr, "Int64QueuePop: popping from an empty queue.\n");
        return 0;
    }
    int64_t element = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    --queue->size;
    return element;
}
