#ifndef GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_BITSTREAM_H_
#define GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_BITSTREAM_H_

#include <stdint.h>   // uint8_t, int64_t

typedef struct BitStream {
    uint8_t *stream;
    int64_t size;
    int64_t num_bytes;
} BitStream;

int BitStreamInit(BitStream *stream, int64_t size);
void BitStreamDestroy(BitStream *stream);

int BitStreamSet(BitStream *stream, int64_t i);
int BitStreamClear(BitStream *stream, int64_t i);

int BitStreamGet(const BitStream *stream, int64_t i);

#endif  // GAMESMANEXPERIMENT_CORE_DATA_STRUCTURES_BITSTREAM_H_
