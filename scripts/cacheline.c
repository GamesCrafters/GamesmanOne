/*
 * cacheline.c – print the hardware cache-line size if the OS exposes it.
 *
 * Build:
 *   gcc -std=c11 -Wall -O2 cacheline.c -o cacheline   (Linux/BSD/macOS)
 *   cl /EHsc cacheline.c                              (Windows, MSVC)
 */
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || \
    defined(__NetBSD__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

#if defined(__linux__)
#include <errno.h>
#include <unistd.h>
#endif

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#include <cpuid.h>
#endif

static size_t cache_line_size(void) {
    /* ---------- 1. Windows --------------------------------------------- */
#if defined(_WIN32)
    DWORD len = 0;
    if (!GetLogicalProcessorInformation(NULL, &len) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buf = malloc(len);
        if (!buf) return 0;
        if (GetLogicalProcessorInformation(buf, &len)) {
            DWORD n = len / sizeof(*buf);
            for (DWORD i = 0; i < n; ++i) {
                if (buf[i].Relationship == RelationCache) {
                    if (buf[i].Cache.Level == 1) {
                        size_t sz = buf[i].Cache.LineSize;
                        free(buf);
                        if (sz) return sz;
                    }
                }
            }
        }
        free(buf);
    }
#endif /* _WIN32 */

    /* ---------- 2. BSD / macOS ----------------------------------------- */
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || \
    defined(__NetBSD__)
    {
        size_t line = 0;
        size_t len = sizeof(line);
        if (sysctlbyname("hw.cachelinesize", &line, &len, NULL, 0) == 0 && line)
            return line;
    }
#endif

    /* ---------- 3. POSIX sysconf --------------------------------------- */
#if defined(_SC_LEVEL1_DCACHE_LINESIZE)
    {
        long res = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
        if (res > 0) return (size_t)res;
    }
#endif

    /* ---------- 4. Linux sysfs ----------------------------------------- */
#if defined(__linux__)
    {
        FILE *f = fopen(
            "/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size",
            "r");
        if (f) {
            long val = 0;
            if (fscanf(f, "%ld", &val) == 1 && val > 0) {
                fclose(f);
                return (size_t)val;
            }
            fclose(f);
        }
    }
#endif

    /* ---------- 5. x86 CPUID ------------------------------------------- */
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    {
        unsigned int eax, ebx, ecx, edx;
        if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
            /* CLFLUSH line size (bits 15-8 of EBX) is in 8-byte units.    */
            unsigned int line = ((ebx >> 8) & 0xff) * 8;
            if (line) return (size_t)line;
        }
    }
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    {
        int info[4] = {0};
        __cpuid(info, 1);
        unsigned int line = ((info[1] >> 8) & 0xff) * 8;
        if (line) return (size_t)line;
    }
#endif

    /* ---------- 6. Conservative default -------------------------------- */
    return 64; /* 64 B covers nearly all modern CPUs               */
}

int main(void) {
    printf("%zu", cache_line_size());
    return 0;
}
