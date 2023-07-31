#ifndef GAMESMANEXPERIMENT_CORE_GAMESMAN_MATH_H_
#define GAMESMANEXPERIMENT_CORE_GAMESMAN_MATH_H_

#include <stdbool.h>  // bool
#include <stdint.h>   // int64_t

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

/**
 * @brief Returns a+b, or -1 if either a or b is negative or if a+b overflows.
 */
int64_t SafeAddNonNegativeInt64(int64_t a, int64_t b);

/**
 * @brief Returns a+b, or -1 if either a or b is negative or if a+b overflows.
 */
int64_t SafeMultiplyNonNegativeInt64(int64_t a, int64_t b);

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

#endif  // GAMESMANEXPERIMENT_CORE_GAMESMAN_MATH_H_
