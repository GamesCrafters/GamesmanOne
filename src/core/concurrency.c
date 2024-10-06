#include "core/concurrency.h"

#ifdef _OPENMP
#include <stdatomic.h>
#endif

#include <stdbool.h>

void ConcurrentBoolInit(ConcurrentBool *cb, bool val) {
#ifdef _OPENMP
    atomic_init(cb, val);
#else
    *cb = val;
#endif
}

bool ConcurrentBoolLoad(const ConcurrentBool *cb) {
#ifdef _OPENMP
    bool ret = atomic_load(cb);
#else
    bool ret = *cb;
#endif

    return ret;
}

void ConcurrentBoolStore(ConcurrentBool *cb, bool val) {
#ifdef _OPENMP
    atomic_store(cb, val);
#else
    *cb = val;
#endif
}

void ConcurrentIntInit(ConcurrentInt *ci, int val) {
#ifdef _OPENMP
    atomic_init(cb, val);
#else
    *ci = val;
#endif
}

int ConcurrentIntLoad(const ConcurrentInt *ci) {
#ifdef _OPENMP
    int ret = atomic_load(ci);
#else
    int ret = *ci;
#endif

    return ret;
}

void ConcurrentIntStore(ConcurrentInt *ci, int val) {
#ifdef _OPENMP
    atomic_store(ci, val);
#else
    *ci = val;
#endif
}
