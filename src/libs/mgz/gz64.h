#ifndef GZ64_H
#define GZ64_H
#include <stdint.h>
#include <zlib.h>

int64_t gz64_read(gzFile file, voidp buf, uint64_t len);

#endif  // GZ64_H
