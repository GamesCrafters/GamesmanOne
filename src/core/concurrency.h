#ifndef GAMESMANONE_CORE_CONCURRENCY_H_
#define GAMESMANONE_CORE_CONCURRENCY_H_

#include <stdbool.h>

#ifdef _OPENMP

#include <stdatomic.h>

#define PRAGMA(X) _Pragma(#X)

#define PRAGMA_OMP_PARALLEL PRAGMA(omp parallel)
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp for schedule(dynamic, k))

#define PRAGMA_OMP_PARALLEL_FOR PRAGMA(omp parallel for)
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k) PRAGMA(omp parallel for schedule(dynamic, k))

#define PRAGMA_OMP_CRITICAL(name) PRAGMA(omp critical(name))

typedef atomic_bool ConcurrentBool;
typedef atomic_int ConcurrentInt;

#else  // _OPENMP not defined.

// The following macros do nothing.

#define PRAGMA_OMP_PARALLEL
#define PRAGMA_OMP_FOR_SCHEDULE_DYNAMIC(k)

#define PRAGMA_OMP_PARALLEL_FOR
#define PRAGMA_OMP_PARALLEL_FOR_SCHEDULE_DYNAMIC(k)

#define PRAGMA_OMP_CRITICAL(name)

typedef bool ConcurrentBool;
typedef int ConcurrentInt;

#endif  // _OPENMP

void ConcurrentBoolInit(ConcurrentBool *cb, bool val);
bool ConcurrentBoolLoad(const ConcurrentBool *cb);
void ConcurrentBoolStore(ConcurrentBool *cb, bool val);

void ConcurrentIntInit(ConcurrentInt *ci, int val);
int ConcurrentIntLoad(const ConcurrentInt *ci);
void ConcurrentIntStore(ConcurrentInt *ci, int val);

#endif  // GAMESMANONE_CORE_CONCURRENCY_H_
