#include "libs/mgz/gz64.h"

#define GZ_READ_CHUNK_SIZE INT_MAX

/* Wrapper function around gzread using 64-bit unsigned integer
   as read size and 64-bit signed integer as return type to
   allow the reading of more than INT_MAX bytes. */
int64_t gz64_read(gzFile file, voidp buf, uint64_t len) {
    int64_t total = 0;
    int bytesRead = 0;

    /* Read INT_MAX bytes at a time. */
    while (len > (uint64_t)GZ_READ_CHUNK_SIZE) {
        bytesRead = gzread(file, buf, GZ_READ_CHUNK_SIZE);
        if (bytesRead != GZ_READ_CHUNK_SIZE) return (int64_t)bytesRead;

        total += bytesRead;
        len -= bytesRead;
        buf = (voidp)((uint8_t*)buf + bytesRead);
    }

    /* Read the rest. */
    bytesRead = gzread(file, buf, (unsigned int)len);
    if (bytesRead != (int)len) return (int64_t)bytesRead;

    return total + bytesRead;
}
