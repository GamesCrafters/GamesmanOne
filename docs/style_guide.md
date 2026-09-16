# GamesmanOne C Style Guide

This document defines the C coding style and best practices for **GamesmanOne**. 

GamesmanOne is primarily a C project (targeting C17) with select C++ components (such as GoogleTest unit tests and benchmarks). The codebase adheres to a modified version of the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) adapted to modern C idioms, constraints, and GamesmanOne conventions.

---

## 1. Key Modifications from Google C++ Style Guide

Several rules from the Google C++ Style Guide are adapted or modified:

1. **4-Space Indentation:** We use 4 spaces per indentation level instead of 2.
2. **Right-Associated Pointer Asterisks:** Asterisks associate to the right (adjacent to the identifier name, e.g., `int *ptr;`), rather than to the left (e.g., `int* ptr;`).
3. **Macro Usage and Naming:** Macros are permitted for constants, compile-time configurations, static assertions, and function-like helpers. Both constant expressions and function-like macros MUST follow `SCREAMING_SNAKE_CASE`.
4. **Function Pointers:** Function pointers are permitted (e.g., in solver APIs, dispatch tables, and sorting comparators). Their identifiers follow the same `PascalCase` rule as functions.
5. **Mandatory / Strongly Recommended Braces for Control Blocks:** All `if`, `else`, `for`, `while`, and `do` bodies should be enclosed in braces `{}` even if they contain only a single statement. This ensures unambiguous debugger stepping (step-over breakpoints), clear line coverage / LCOV visualization, and prevention of dangling-`else` bugs.
6. **C-Style Namespacing:** Since C lacks namespaces, module-level prefixing (e.g., `Int64Array...`, `Gamesman...`, `TierSolver...`) is used for public functions and types.
7. **Opaque Data Types:** Object-oriented encapsulation is achieved via opaque struct pointers (Abstract Data Types) and explicit constructor/destructor functions.

---

## 2. Formatting and Layout

### 2.1 Indentation
* Use **4 spaces** per indentation level.
* Do **not** use tab characters (`\t`).

```c
// Good
if (condition) {
    DoSomething();
}

// Bad
if (condition) {
  DoSomething(); // 2 spaces
}
```

### 2.2 Line Length
* The project-wide line length limit is **80 columns**.
* Lines exceeding 80 columns should be cleanly wrapped, aligning continuation lines logically:

```c
// Good
bool Int64ArrayInitCopy(Int64Array *dest, const Int64Array *src);

ConcurrentBitset *ConcurrentBitsetCreateAllocatorMt(
    int64_t num_bits, GamesmanAllocator *allocator);
```

### 2.3 Braces and Control Structures
* Follow the **1TBS (One True Brace Style) / K&R style**:
  * Opening braces `{` appear on the same line as the statement or function declaration.
  * Closing braces `}` appear on their own line matching the indentation of the initiating statement.
  * `else` and `else if` appear on the same line as the preceding closing brace (`} else {`).
* **Always put braces around single-line statement bodies:**

```c
// Good
if (ret != NULL) {
    ret->num_bits = num_bits;
} else {
    return false;
}

// Bad
if (ret != NULL)
    ret->num_bits = num_bits;
```

* For `switch` statements, indent `case` labels by 4 spaces, and their associated code blocks by an additional 4 spaces:

```c
switch (value) {
    case kWin:
        CountWin(analysis, tier_position, remoteness, is_canonical);
        break;

    case kLose:
        CountLose(analysis, tier_position, remoteness, is_canonical);
        break;

    default:
        return kIllegalGamePositionValueError;
}
```

### 2.4 Pointer Asterisks
* Place the asterisk adjacent to the variable, parameter, or function pointer name (**right-associated**):

```c
// Good
int *ptr;
const char *buffer;
void *GamesmanMalloc(size_t size);
int (*comp)(const void *, const void *);
(int64_t *)space;

// Bad
int* ptr;
const char* buffer;
void* GamesmanMalloc(size_t size);
(int64_t*) space;
```

### 2.5 Horizontal Spacing
* Place a space between keywords and opening parentheses: `if (condition)`, `while (loop)`, `switch (val)`.
* Do **not** place a space between function names and opening parentheses: `Int64ArrayInit(array);`.
* Place one space around binary and ternary operators: `=`, `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `>`, `&&`, `||`, `?`, `:`.
* Do **not** place spaces around unary operators: `++`, `--`, `!`, `~`, `&` (address-of), `*` (dereference), `sizeof`.
* Do **not** place spaces immediately inside parentheses: `(key != NULL)` (not `( key != NULL )`).

### 2.6 Vertical Spacing (Blank Lines)
* Separate function definitions with a single blank line.
* Group logical paragraphs within functions using single blank lines.
* Leave exactly one blank line before out-of-line Doxygen comments, except for the file header at the very top of the file.

---

## 3. Naming Conventions

| Category | Style | Examples |
| :--- | :--- | :--- |
| **Types & Structs** | `PascalCase` | `Int64Array`, `GamesmanAllocator`, `TierPosition`, `Value` |
| **Functions** | `PascalCase` | `Int64ArrayPushBack`, `BitsetCreate`, `GamesmanMalloc` |
| **Function Pointers** | `PascalCase` | `GetInitialTier`, `Primitive`, `DoMove` |
| **Local Variables & Parameters** | `snake_case` | `num_bits`, `alloc_size`, `new_capacity`, `dest`, `src` |
| **Struct & Union Members** | `snake_case` | `array->size`, `allocator->pool_size`, `hs->capacity_mask` |
| **Constants (Global / File / Enums)** | `kPascalCase` | `kBitsPerBlock`, `kDefaultTier`, `kSuccess`, `kWin` |
| **Macros (Constants & Functions)** | `SCREAMING_SNAKE_CASE` | `GM_CACHE_LINE_SIZE`, `INT64_HASH_SET_EMPTY_KEY`, `PRAGMA_OMP` |
| **Source & Header Files** | `snake_case` | `int64_array.c`, `int64_array.h`, `tier_solver.h` |

### 3.1 Types and Structs
* Type names (including structs, enums, unions, and typedefs) must use `PascalCase`.
* Typedef struct definitions should typically match the struct tag:
```c
typedef struct Int64Array {
    GamesmanAllocator *allocator;
    int64_t *array;
    int64_t size;
    int64_t capacity;
} Int64Array;
```
* For opaque types, declare the forward typedef in the header:
```c
typedef struct Bitset Bitset;
```

### 3.2 Functions and Function Pointers
* Function names must use `PascalCase`.
* Standard naming convention is `<Module><Action>` or `<Type><Action>`:
```c
void Int64ArrayInit(Int64Array *array);
bool Int64ArrayPushBack(Int64Array *array, int64_t item);
void *GamesmanMalloc(size_t size);
```
* Function pointers follow the same naming convention as functions:
```c
typedef struct TierSolverApi {
    Tier (*GetInitialTier)(void);
    int (*GenerateMoves)(TierPosition tier_position, Move moves[]);
    Position (*DoMove)(TierPosition tier_position, Move move);
} TierSolverApi;
```

### 3.3 Constants and Enumerators
* Named constants with static storage duration and enum values must begin with a lowercase `k` followed by `PascalCase`:
```c
static const int64_t kMinimumCapacity = 16;

typedef enum Value {
    kUndecided = 0,
    kLose,
    kDraw,
    kTie,
    kWin,
    kNumValues,
} Value;
```

### 3.4 Macros
* All macros (whether representing constant expressions, sentinels, or function-like macros) must use `SCREAMING_SNAKE_CASE`:
```c
#define INT64_HASH_SET_EMPTY_KEY INT64_MIN
#define GM_CACHE_LINE_PAD(n) ((((n) + (GM_CACHE_LINE_SIZE) - 1) / (GM_CACHE_LINE_SIZE) * (GM_CACHE_LINE_SIZE)) - (n))
#define DECLARE_INT64_STATIC_HASH_SET(name, cap) ...
```

---

## 4. Header Files and Source Organization

### 4.1 Header Guards
* Header guards must be named in the format `GAMESMANONE_<DIR>_<SUBDIR>_<FILENAME>_H_`.
* The trailing `#endif` must include a comment specifying the guard name preceded by two spaces:

```c
#ifndef GAMESMANONE_CORE_DATA_STRUCTURES_INT64_ARRAY_H_
#define GAMESMANONE_CORE_DATA_STRUCTURES_INT64_ARRAY_H_

// Header contents...

#endif  // GAMESMANONE_CORE_DATA_STRUCTURES_INT64_ARRAY_H_
```

### 4.2 Include Ordering
In `.c` source files, includes must be structured into grouped blocks separated by blank lines in the following order:

1. **Associated Header:** The corresponding `.h` file for the source file.
2. **C Standard Library Headers:** In alphabetical order (e.g., `<assert.h>`, `<stdbool.h>`, `<stdint.h>`, `<stdio.h>`, `<stdlib.h>`, `<string.h>`).
3. **POSIX / System / Third-Party Headers:** (e.g., `<unistd.h>`, `<omp.h>`, `<lzma.h>`).
4. **Project Headers:** In alphabetical order from the root (e.g., `"config.h"`, `"core/concurrency.h"`, `"core/gamesman_memory.h"`).

```c
#include "core/data_structures/int64_array.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "core/gamesman_memory.h"
```

### 4.3 Include Paths
* All project headers must be included using paths relative to the project root:
```c
// Good
#include "core/types/base.h"
#include "core/data_structures/hash.h"

// Bad
#include "../types/base.h"
#include "hash.h"
```

### 4.4 C++ Compatibility
* Header files that may be consumed by C++ translation units (e.g. tests, benchmarks) should include `extern "C"` wrappers:

```c
#ifdef __cplusplus
extern "C" {
#endif

// Declarations...

#ifdef __cplusplus
}
#endif
```

---

## 5. Modern C Language Practices

### 5.1 Fixed-Width Types
* Use fixed-width integer types from `<stdint.h>` (`int64_t`, `uint64_t`, `int32_t`, `uint8_t`) and `<stddef.h>` (`size_t`, `ptrdiff_t`) when integer size and sign matter.
* Avoid raw `int` or `long` for data structure sizes, capacities, positions, and offsets. Standard `int` is acceptable for small loop counters, standard library wrappers, or boolean return codes.
* Use `<stdbool.h>` (`bool`, `true`, `false`) for all boolean logic.
* Use `<inttypes.h>` format specifier macros (e.g., `PRId64`, `PRIu64`) when printing fixed-width types:
```c
printf("Position: %" PRId64 "\n", pos);
```

### 5.2 Static Inline Functions
* Small, performance-critical accessor or inline manipulation functions should be marked `static inline` in header files:
```c
static inline bool Int64ArrayEmpty(const Int64Array *array) {
    return array->size == 0;
}
```

### 5.3 Static Assertions
* Use `static_assert` from `<assert.h>` to validate compile-time invariants, configuration assumptions, and struct alignments:
```c
static_assert((GM_CACHE_LINE_SIZE & (GM_CACHE_LINE_SIZE - 1)) == 0,
              "GM_CACHE_LINE_SIZE is not defined as a power of 2");
```

### 5.4 Macro Safety
* When defining macros with parameters, enclose all parameter usages and the entire macro expression in parentheses to avoid operator precedence issues:
```c
#define GM_CACHE_LINE_PAD(n)                                    \
    ((((n) + (GM_CACHE_LINE_SIZE) - 1) / (GM_CACHE_LINE_SIZE) * \
      (GM_CACHE_LINE_SIZE)) -                                   \
     (n))
```
* Prefer `static inline` functions over function-like macros whenever feasible.

### 5.5 Array Bounds and Restrict Qualifiers
* Use array parameter size bounds (`[static N]`) in public API function signatures to document and enforce minimum buffer size guarantees:
```c
int (*GenerateMoves)(TierPosition tier_position,
                     Move moves[static kTierSolverNumMovesMax]);
```
* Use `__restrict` (or `restrict`) for pointers in hot loops where memory aliasing does not occur to assist compiler optimization:
```c
int64_t *__restrict keys = set->keys;
```

---

## 6. Memory Management and Safety

### 6.1 Gamesman Memory Management API
* Allocate and free heap memory using the Gamesman memory management system (`core/gamesman_memory.h`):
  * `GamesmanMalloc(size_t size)`
  * `GamesmanCallocWhole(size_t nmemb, size_t size)`
  * `GamesmanAlignedAlloc(size_t alignment, size_t size)`
  * `GamesmanFree(void *ptr)`
* For objects supporting custom allocators, use `GamesmanAllocatorAllocate` and `GamesmanAllocatorDeallocate`.
* Use `SafeMalloc` / `SafeCalloc` only for unrecoverable startup allocations where failure warrants an immediate abort.

### 6.2 Defensive Memory Checks
* **Always** check the return value of allocation functions for `NULL`:
```c
dest->array = (int64_t *)GamesmanAllocatorAllocate(
    src->allocator, src->size * sizeof(int64_t));
if (dest->array == NULL) {
    return false;
}
```
* Check for arithmetic overflow before calculating allocation sizes:
```c
if (size > SIZE_MAX - header_size) {
    return NULL;
}
```

### 6.3 Destructor and Cleanup Discipline
* Destructors and release functions (e.g. `BitsetDestroy`, `Int64ArrayDestroy`) must be **NULL-safe**: calling them on a `NULL` pointer must be a safe no-op.
* Clear pointers to `NULL` and zero out fields upon destruction:
```c
void Int64ArrayDestroy(Int64Array *array) {
    GamesmanAllocatorDeallocate(array->allocator, array->array);
    GamesmanAllocatorRelease(array->allocator);
    array->allocator = NULL;
    array->array = NULL;
    array->size = 0;
    array->capacity = 0;
}
```

---

## 7. Documentation and Comments

### 7.1 Header Doxygen Comments
* All public functions, structs, enums, macros, and global variables in header files (`.h`) must be documented with Doxygen comments following [Doxygen Conventions](doxygen_conventions.md).
* File headers must contain `@file`, `@author`, `@brief`, and `@copyright` in that exact order.
* Function documentation must use `@param[in]`, `@param[out]`, `@param[in,out]`, `@return`, and `@retval`:

```c
/**
 * @brief Pushes a new `item` to the back of the `array`.
 *
 * @param[in,out] array Destination array.
 * @param[in] item New item.
 *
 * @retval true on success.
 * @retval false otherwise.
 */
static inline bool Int64ArrayPushBack(Int64Array *array, int64_t item);
```

### 7.2 Implementation Comments
* Comment the **why**, not the **what**. The code should be self-documenting for routine operations; use inline comments to clarify non-obvious algorithms, performance optimizations, or edge-case handling.
* Use section banners for large files to delineate modules:
```c
// ================================= Allocator =================================
```


---

## 8. Unit Testing and Verification

* For writing unit tests in C++, adhere to the [Unit Testing Conventions](unit_testing_conventions.md):
  * Use `PascalCase` without underscores for both test suite names and test names in `TEST(TestSuiteName, TestName)`.
  * Ensure test isolation for parallel test runs (`ctest -j`).
  * Target edge cases: zero/null values, empty collections, numerical boundaries, idempotency, and lifecycle destruction.
