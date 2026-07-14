#ifndef GAMESMANONE_LIBS_MATH_COMBINATORICS_H_
#define GAMESMANONE_LIBS_MATH_COMBINATORICS_H_

#include <stdint.h>

/**
 * @brief Returns the number of ways to choose R elements from a total of N
 * elements.
 *
 * @param n Positive integer, number of elements to choose from.
 * @param r Positive integer, number of elements to choose.
 * @return Returns nCr(N, R) if the result can be expressed as a 64-bit
 * signed integer. Returns -1 if either N or R is negative or if the result
 * overflows.
 */
int64_t NChooseR(int n, int r);

#endif  // GAMESMANONE_LIBS_MATH_COMBINATORICS_H_
