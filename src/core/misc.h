#ifndef GAMESMANEXPERIMENT_CORE_MISC_H_
#define GAMESMANEXPERIMENT_CORE_MISC_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t

#include "core/gamesman_types.h"

void NotReached(const char *message);
void *SafeMalloc(size_t size);
void *SafeCalloc(size_t n, size_t size);

/**
 * @brief Recursively makes all directories along the given path.
 * Equivalent to "mkdir -p <path>".
 *
 * @param path Make all directories along this path.
 * @return 0 on success. On error, -1 is returned and errno is set to indicate
 * the error.
 *
 * @authors Jonathon Reinhart and Carl Norum
 * @link http://stackoverflow.com/a/2336245/119527,
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
int MkdirRecursive(const char *path);

void PositionArrayInit(PositionArray *array);
void PositionArrayDestroy(PositionArray *array);
bool PositionArrayAppend(PositionArray *array, Position position);
bool PositionArrayContains(PositionArray *array, Position position);

void PositionHashSetInit(PositionHashSet *set, double max_load_factor);
void PositionHashSetDestroy(PositionHashSet *set);
bool PositionHashSetContains(PositionHashSet *set, Position position);
bool PositionHashSetAdd(PositionHashSet *set, Position position);

void MoveArrayInit(MoveArray *array);
void MoveArrayDestroy(MoveArray *array);
bool MoveArrayAppend(MoveArray *array, Move move);
bool MoveArrayPopBack(MoveArray *array);
bool MoveArrayContains(const MoveArray *array, Move move);

void TierArrayInit(TierArray *array);
void TierArrayDestroy(TierArray *array);
bool TierArrayAppend(TierArray *array, Tier tier);

void TierStackInit(TierStack *stack);
void TierStackDestroy(TierStack *stack);
bool TierStackPush(TierStack *stack, Tier tier);
void TierStackPop(TierStack *stack);
Tier TierStackTop(const TierStack *stack);
bool TierStackEmpty(const TierStack *stack);

void TierQueueInit(TierQueue *queue);
void TierQueueDestroy(TierQueue *queue);
bool TierQueueIsEmpty(const TierQueue *queue);
int64_t TierQueueSize(const TierQueue *queue);
bool TierQueuePush(TierQueue *queue, Tier tier);
Tier TierQueuePop(TierQueue *queue);

void TierHashMapInit(TierHashMap *map, double max_load_factor);
void TierHashMapDestroy(TierHashMap *map);
TierHashMapIterator TierHashMapGet(TierHashMap *map, Tier key);
bool TierHashMapSet(TierHashMap *map, Tier tier, int64_t value);
bool TierHashMapContains(TierHashMap *map, Tier tier);

void TierHashMapIteratorInit(TierHashMapIterator *it, TierHashMap *map);
Tier TierHashMapIteratorKey(const TierHashMapIterator *it);
int64_t TierHashMapIteratorValue(const TierHashMapIterator *it);
bool TierHashMapIteratorIsValid(const TierHashMapIterator *it);
bool TierHashMapIteratorNext(TierHashMapIterator *iterator, Tier *tier,
                             int64_t *value);

void TierHashSetInit(TierHashSet *set, double max_load_factor);
void TierHashSetDestroy(TierHashSet *set);
bool TierHashSetContains(TierHashSet *set, Tier tier);
bool TierHashSetAdd(TierHashSet *set, Tier tier);

void TierPositionArrayInit(TierPositionArray *array);
void TierPositionArrayDestroy(TierPositionArray *array);
bool TierPositionArrayAppend(TierPositionArray *array,
                             TierPosition tier_position);
TierPosition TierPositionArrayBack(const TierPositionArray *array);

void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor);
void TierPositionHashSetDestroy(TierPositionHashSet *set);
bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key);
bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key);

int GameVariantToIndex(const GameVariant *variant);

#endif  // GAMESMANEXPERIMENT_CORE_MISC_H_
