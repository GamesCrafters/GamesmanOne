#ifndef GAMESMANEXPERIMENT_CORE_MISC_H_
#define GAMESMANEXPERIMENT_CORE_MISC_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

#include "core/gamesman_types.h"

void NotReached(const char *message);

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
bool TierPositionArrayAdd(TierPositionArray *array, TierPosition tier_position);

void TierPositionHashSetInit(TierPositionHashSet *set, double max_load_factor);
void TierPositionHashSetDestroy(TierPositionHashSet *set);
bool TierPositionHashSetContains(TierPositionHashSet *set, TierPosition key);
bool TierPositionHashSetAdd(TierPositionHashSet *set, TierPosition key);

/**
 * @brief Tests if N is prime. Returns false if N is non-possitive.
 *
 * @param n Integer.
 * @return True if N is a positive prime number, false otherwise.
 * @author Naman_Garg, geeksforgeeks.org.
 * https://www.geeksforgeeks.org/program-to-find-the-next-prime-number/
 */
bool IsPrime(int64_t n);

/**
 * @brief Returns the largest prime number that is smaller than
 * or equal to N, unless N is less than 2, in which case 2 is
 * returned.
 *
 * @param n Reference.
 * @return Previous prime of N.
 */
int64_t PrevPrime(int64_t n);

/**
 * @brief Returns the smallest prime number that is greater than
 * or equal to N, assuming no integer overflow occurs.
 *
 * @param n Reference.
 * @return Next prime of N.
 */
int64_t NextPrime(int64_t n);

#endif  // GAMESMANEXPERIMENT_CORE_MISC_H_
