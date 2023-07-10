#ifndef GAMESMANEXPERIMENT_CORE_MISC_H_
#define GAMESMANEXPERIMENT_CORE_MISC_H_

#include <stdbool.h>
#include <stdint.h>

#include "gamesman_types.h"

void PositionArrayInit(PositionArray *array);
void PositionArrayDestroy(PositionArray *array);
bool PositionArrayAppend(PositionArray *array, Position position);
bool PositionArrayContains(PositionArray *array, Position position);

void MoveListDestroy(MoveList list);

void TierArrayInit(TierArray *array);
void TierArrayDestroy(TierArray *array);
bool TierArrayAppend(TierArray *array, Tier tier);

void TierStackInit(TierStack *stack);
void TierStackDestroy(TierStack *stack);
bool TierStackPush(TierStack *stack, Tier tier);
void TierStackPop(TierStack *stack);
Tier TierStackTop(TierStack *stack);
bool TierStackEmpty(TierStack *stack);

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
