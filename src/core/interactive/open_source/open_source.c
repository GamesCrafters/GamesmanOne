#include "core/interactive/open_source/open_source.h"

#include <stdio.h>

#include "core/types/base.h"

typedef struct OpenSourceSoftware {
    char name[256];
    char url[2048];
} OpenSourceSoftware;

// clang-format off
static const OpenSourceSoftware kSoftwareList[] = {
    {
        .name = "Benchmark",
        .url= "https://github.com/google/benchmark",
    },
    {
        .name = "GoogleTest",
        .url= "https://github.com/google/googletest",
    },
    {
        .name = "JSON-C - A JSON implementation in C",
        .url = "https://github.com/json-c/json-c",
    },
    {
        .name = "XZ Utils",
        .url = "https://github.com/tukaani-project/xz",
    },
    {
        .name = "LZ4 - Extremely fast compression",
        .url = "https://github.com/lz4/lz4",
    },
    {
        .name = "MT19937-64",
        .url = "https://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/VERSIONS/C-LANG/mt19937-64.c",
    },
    {
        .name = "CityHash",
        .url = "https://github.com/google/cityhash",
    },
    {
        .name = "folly",
        .url = "https://github.com/facebook/folly",
    },
};
// clang-format on

int InteractiveOpenSource(ReadOnlyString key) {
    (void)key;
    printf("Open Source Software Usage:\n");
    const int length = sizeof(kSoftwareList) / sizeof(kSoftwareList[0]);
    for (int i = 0; i < length; ++i) {
        printf("  - %s: <%s>\n", kSoftwareList[i].name, kSoftwareList[i].url);
    }

    return 0;
}
