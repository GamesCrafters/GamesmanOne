#include "core/misc.h"

#include <errno.h>      // errno
#include <stdio.h>      // fprintf, stderr
#include <stdlib.h>     // exit, malloc, calloc, free
#include <string.h>     // strlen, strncpy
#include <sys/stat.h>   // mkdir, struct stat
#include <sys/types.h>  // mode_t

void NotReached(const char *message) {
    fprintf(stderr,
            "(FATAL) You entered a branch that is marked as NotReached. The "
            "error message was %s\n",
            message);
    exit(EXIT_FAILURE);
}

void *SafeMalloc(size_t size) {
    void *ret = malloc(size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeMalloc: failed to allocate %zd bytes. This ususally "
                "indicates a bug.\n",
                size);
        exit(EXIT_FAILURE);
    }
    return ret;
}

void *SafeCalloc(size_t n, size_t size) {
    void *ret = calloc(n, size);
    if (ret == NULL) {
        fprintf(stderr,
                "SafeCalloc: failed to allocate %zd elements each of %zd "
                "bytes. This ususally "
                "indicates a bug.\n",
                n, size);
        exit(EXIT_FAILURE);
    }
    return ret;
}

/**
 * @brief Makes a directory at the given path or does nothing if the directory
 * already exists.
 *
 * @return 0 on success. On error, -1 is returned and errno is set to indicate
 * the error.
 *
 * @author Jonathon Reinhart
 * @link
 * https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
 */
static int MaybeMkdir(const char *path, mode_t mode) {
    errno = 0;

    // Try to make the directory
    if (mkdir(path, mode) == 0) return 0;

    // If it fails for any reason but EEXIST, fail
    if (errno != EEXIST) return -1;

    // Check if the existing path is a directory
    struct stat st;
    if (stat(path, &st) != 0) return -1;

    // If not, fail with ENOTDIR
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    errno = 0;
    return 0;
}

int MkdirRecursive(const char *path) {
    // Fail if path is NULL.
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    int result = -1;
    errno = 0;

    // Copy string so it's mutable
    size_t path_length = strlen(path);
    char *path_copy = (char *)malloc((path_length + 1) * sizeof(char));
    if (path_copy == NULL) {
        errno = ENOMEM;
        goto _bailout;
    }
    strncpy(path_copy, path, path_length + 1);

    for (size_t i = 0; i < path_length; ++i) {
        if (path_copy[i] == '/') {
            path_copy[i] = '\0';  // Temporarily truncate
            if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
            path_copy[i] = '/';
        }
    }
    if (MaybeMkdir(path_copy, 0777) != 0) goto _bailout;
    result = 0;  // Success

_bailout:
    free(path_copy);
    return result;
}
