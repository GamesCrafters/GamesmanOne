#ifndef GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_QUEUE_H_
#define GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_QUEUE_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_ts

typedef struct {
    int64_t *array;
    int64_t front;
    int64_t size;
    int64_t capacity;
} Int64Queue;

void Int64QueueInit(Int64Queue *queue);
void Int64QueueDestroy(Int64Queue *queue);

bool Int64QueueIsEmpty(const Int64Queue *queue);
int64_t Int64QueueSize(const Int64Queue *queue);

bool Int64QueuePush(Int64Queue *queue, int64_t element);
int64_t Int64QueuePop(Int64Queue *queue);

#endif  // GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_INT64_QUEUE_H_
