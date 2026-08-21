#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "core/data_structures/cstring.h"
}

// ============================== CStringGetNull ==============================

// Verifies that CStringGetNull returns a properly zeroed-out CString struct.
TEST(CStringTest, GetNullReturnsNullState) {
    CString cstr = CStringGetNull();

    EXPECT_EQ(cstr.str, nullptr)
        << "The string pointer must be null for a null state.";
    EXPECT_EQ(cstr.length, 0) << "The length must be 0 for a null state.";
    EXPECT_EQ(cstr.capacity, 0) << "The capacity must be 0 for a null state.";
}

// ============================= CStringInitEmpty =============================

// Verifies that initializing an empty string sets up a valid null-terminated
// string of length zero with strictly positive allocated capacity.
TEST(CStringTest, InitEmptyInitializesValidEmptyString) {
    CString cstr;
    bool result = CStringInitEmpty(&cstr);

    ASSERT_TRUE(result) << "CStringInitEmpty must return true on success.";
    ASSERT_NE(cstr.str, nullptr)
        << "The string pointer must be allocated and not null.";
    EXPECT_EQ(cstr.str[0], '\0')
        << "The first character must be the null terminator.";
    EXPECT_EQ(cstr.length, 0) << "An empty string must have a length of 0.";
    EXPECT_GT(cstr.capacity, 0)
        << "Capacity must be > 0 to hold at least the null terminator.";

    CStringDestroy(&cstr);
}

// Verifies that CStringInitEmpty safely rejects a null pointer parameter
// without causing a segmentation fault.
TEST(CStringTest, InitEmptyFailsGracefullyOnNullPointer) {
    bool result = CStringInitEmpty(nullptr);

    EXPECT_FALSE(result)
        << "CStringInitEmpty must return false when provided a null pointer.";
}

// ============================== CStringInitCopy ==============================

// Verifies that copying a populated string performs a deep copy of the buffer
// and accurately transfers length and capacity data.
TEST(CStringTest, InitCopyCopiesPopulatedString) {
    CString source;
    source.length = 5;
    source.capacity = 6;
    source.str = static_cast<char*>(std::malloc(6));
    std::strcpy(source.str, "hello");

    CString target;
    bool result = CStringInitCopy(&target, &source);

    ASSERT_TRUE(result) << "CStringInitCopy must return true on successful "
                           "allocation and copy.";
    EXPECT_NE(target.str, source.str)
        << "A deep copy must allocate a new buffer, not copy the pointer.";
    EXPECT_STREQ(target.str, source.str)
        << "The string contents must match the source exactly.";
    EXPECT_EQ(target.length, source.length)
        << "The target length must match the source length.";
    EXPECT_GE(target.capacity, target.length)
        << "The target capacity must be at least its length.";

    std::free(source.str);
    CStringDestroy(&target);
}

// Verifies that copying an empty string creates a valid target string
// in the empty state, not the null state.
TEST(CStringTest, InitCopyCopiesEmptyString) {
    CString source;
    source.length = 0;
    source.capacity = 1;
    source.str = static_cast<char*>(std::malloc(1));
    source.str[0] = '\0';

    CString target;
    bool result = CStringInitCopy(&target, &source);

    ASSERT_TRUE(result) << "CStringInitCopy must return true when copying a "
                           "valid empty string.";
    ASSERT_NE(target.str, nullptr)
        << "An empty string must have an allocated buffer.";
    EXPECT_NE(target.str, source.str)
        << "A deep copy must allocate a new buffer even for empty strings.";
    EXPECT_EQ(target.str[0], '\0')
        << "The target must be properly null-terminated.";
    EXPECT_EQ(target.length, 0) << "The target length must be 0.";
    EXPECT_GE(target.capacity, 1)
        << "The target capacity must be at least 1 for the null terminator.";

    std::free(source.str);
    CStringDestroy(&target);
}

// Verifies that attempting to copy a string in the null state safely
// initializes the target string to the null state as well.
TEST(CStringTest, InitCopyHandlesNullSourceString) {
    CString source = CStringGetNull();
    CString target;

    bool result = CStringInitCopy(&target, &source);

    EXPECT_TRUE(result) << "Copying a null string state should succeed and "
                           "yield a null string.";
    EXPECT_EQ(target.str, nullptr)
        << "Target must be initialized to a null pointer.";
    EXPECT_EQ(target.length, 0) << "Target length must be 0.";
    EXPECT_EQ(target.capacity, 0) << "Target capacity must be 0.";
}

// Verifies that passing a null pointer as the source safely initializes
// the target to the null state.
TEST(CStringTest, InitCopyHandlesNullSourcePointer) {
    CString target;
    bool result = CStringInitCopy(&target, nullptr);

    EXPECT_TRUE(result) << "CStringInitCopy should handle a null source "
                           "pointer by treating it as a null state.";
    EXPECT_EQ(target.str, nullptr)
        << "Target must be initialized to a null pointer.";
    EXPECT_EQ(target.length, 0) << "Target length must be 0.";
    EXPECT_EQ(target.capacity, 0) << "Target capacity must be 0.";
}

// Verifies that passing a null pointer as the target safely rejects the
// operation and returns false without segfaulting.
TEST(CStringTest, InitCopyFailsOnNullTargetPointer) {
    CString source = CStringGetNull();
    bool result = CStringInitCopy(nullptr, &source);

    EXPECT_FALSE(result)
        << "CStringInitCopy must return false when the target pointer is null.";
}

// ========================= CStringInitCopyCharArray =========================

// Verifies that initializing from a valid C-style array performs a deep copy
// and correctly sets length and capacity.
TEST(CStringTest, InitCopyCharArrayCopiesStandardString) {
    CString target;
    const char* src = "GoogleTest";
    bool result = CStringInitCopyCharArray(&target, src);

    ASSERT_TRUE(result) << "CStringInitCopyCharArray must return true for "
                           "valid source strings.";
    ASSERT_NE(target.str, nullptr)
        << "Target string pointer must be allocated.";
    EXPECT_STREQ(target.str, src)
        << "Target string contents must match the source exactly.";
    EXPECT_EQ(target.length, 10)
        << "Target length must be exactly the length of the source string.";
    EXPECT_GE(target.capacity, target.length)
        << "Target capacity must be at least its length.";

    CStringDestroy(&target);
}

// Verifies that initializing from an empty C-style array results in a valid
// empty CString state.
TEST(CStringTest, InitCopyCharArrayCopiesEmptyCharArray) {
    CString target;
    const char* src = "";
    bool result = CStringInitCopyCharArray(&target, src);

    ASSERT_TRUE(result) << "CStringInitCopyCharArray must return true for "
                           "empty source strings.";
    ASSERT_NE(target.str, nullptr)
        << "Target string pointer must be allocated for an empty string.";
    EXPECT_EQ(target.str[0], '\0')
        << "Target string must be properly null-terminated.";
    EXPECT_EQ(target.length, 0) << "Target length must be 0.";
    EXPECT_GE(target.capacity, 0) << "Target capacity must not be negative.";

    CStringDestroy(&target);
}

// Verifies that passing a null source pointer initializes the target
// to the null state safely.
TEST(CStringTest, InitCopyCharArrayHandlesNullSourcePointer) {
    CString target;
    bool result = CStringInitCopyCharArray(&target, nullptr);

    EXPECT_TRUE(result) << "Initializing from a null pointer should succeed "
                           "and yield a null state.";
    EXPECT_EQ(target.str, nullptr)
        << "Target must be initialized to a null pointer.";
    EXPECT_EQ(target.length, 0) << "Target length must be 0.";
    EXPECT_EQ(target.capacity, 0) << "Target capacity must be 0.";
}

// Verifies that passing a null target pointer rejects the operation
// safely without causing a segmentation fault.
TEST(CStringTest, InitCopyCharArrayFailsOnNullTargetPointer) {
    const char* src = "GoogleTest";
    bool result = CStringInitCopyCharArray(nullptr, src);

    EXPECT_FALSE(result) << "CStringInitCopyCharArray must return false when "
                            "the target pointer is null.";
}

// ============================== CStringInitMove ==============================

// Verifies that moving a populated string transfers the exact memory buffer
// to the target and leaves the source string in the null state.
TEST(CStringTest, InitMoveMovesPopulatedString) {
    CString source;
    ASSERT_TRUE(CStringInitCopyCharArray(&source, "hello"))
        << "Setup failed: could not initialize source string.";

    char* original_ptr = source.str;
    int64_t original_capacity = source.capacity;

    CString target;
    CStringInitMove(&target, &source);

    EXPECT_EQ(target.str, original_ptr)
        << "The target must acquire the exact memory buffer from the source.";
    EXPECT_STREQ(target.str, "hello")
        << "The target contents must remain intact after the move.";
    EXPECT_EQ(target.length, 5)
        << "The target length must match the original source length.";
    EXPECT_EQ(target.capacity, original_capacity)
        << "The target capacity must match the original source capacity.";

    EXPECT_EQ(source.str, nullptr)
        << "The source string pointer must be nulled out after a move.";
    EXPECT_EQ(source.length, 0)
        << "The source string length must be 0 after a move.";
    EXPECT_EQ(source.capacity, 0)
        << "The source string capacity must be 0 after a move.";

    CStringDestroy(&target);
}

// Verifies that moving an empty string transfers its allocated buffer
// and correctly transitions the source to the null state.
TEST(CStringTest, InitMoveMovesEmptyString) {
    CString source;
    ASSERT_TRUE(CStringInitEmpty(&source))
        << "Setup failed: could not initialize empty source string.";

    char* original_ptr = source.str;

    CString target;
    CStringInitMove(&target, &source);

    EXPECT_EQ(target.str, original_ptr)
        << "The target must acquire the exact memory buffer, even for empty "
           "strings.";
    EXPECT_EQ(target.length, 0) << "The target length must be 0.";
    EXPECT_EQ(target.str[0], '\0')
        << "The target must remain properly null-terminated.";

    EXPECT_EQ(source.str, nullptr)
        << "The source string must transition to the null state.";
    EXPECT_EQ(source.length, 0)
        << "The source string length must be 0 after a move.";

    CStringDestroy(&target);
}

// Verifies that attempting to move a string already in the null state
// safely initializes the target to the null state.
TEST(CStringTest, InitMoveHandlesNullSource) {
    CString source = CStringGetNull();
    CString target;

    CStringInitMove(&target, &source);

    EXPECT_EQ(target.str, nullptr)
        << "Moving a null state source must yield a null state target.";
    EXPECT_EQ(target.length, 0) << "Target length must be 0.";
    EXPECT_EQ(target.capacity, 0) << "Target capacity must be 0.";

    EXPECT_EQ(source.str, nullptr) << "Source must remain in the null state.";
}

// Verifies that passing a null pointer as the source safely initializes
// the target to the null state.
TEST(CStringTest, InitMoveHandlesNullSourcePointer) {
    CString target;

    CStringInitMove(&target, nullptr);

    EXPECT_EQ(target.str, nullptr) << "Target must be initialized to a null "
                                      "pointer when source pointer is null.";
    EXPECT_EQ(target.length, 0) << "Target length must be 0.";
    EXPECT_EQ(target.capacity, 0) << "Target capacity must be 0.";
}

// Verifies that moving a string into itself is treated as a safe no-op
// and does not destroy the string's existing data.
TEST(CStringTest, InitMoveHandlesSelfMove) {
    CString target;
    ASSERT_TRUE(CStringInitCopyCharArray(&target, "test"))
        << "Setup failed: could not initialize target string.";

    char* original_ptr = target.str;
    int64_t original_capacity = target.capacity;

    CStringInitMove(&target, &target);

    EXPECT_EQ(target.str, original_ptr)
        << "Self-move must not alter the underlying buffer pointer.";
    EXPECT_STREQ(target.str, "test")
        << "Self-move must not destroy the string contents.";
    EXPECT_EQ(target.length, 4) << "Self-move must leave the length intact.";
    EXPECT_EQ(target.capacity, original_capacity)
        << "Self-move must leave the capacity intact.";

    CStringDestroy(&target);
}

// Verifies that passing a null pointer as the target (init) safely
// aborts the operation without causing a segmentation fault, and leaves
// the source string intact since the move could not be completed.
TEST(CStringTest, InitMoveHandlesNullTargetPointer) {
    CString source;
    ASSERT_TRUE(CStringInitCopyCharArray(&source, "Data"))
        << "Setup failed: could not initialize source string.";

    // If the function does not check for init == nullptr, this will segfault.
    CStringInitMove(nullptr, &source);

    SUCCEED() << "CStringInitMove safely handled a null target pointer without "
                 "crashing.";

    // Since the move could not happen, the source string should remain
    // completely unmodified.
    EXPECT_NE(source.str, nullptr) << "The source string pointer must not be "
                                      "nulled out if the move fails.";
    EXPECT_STREQ(source.str, "Data")
        << "The source string contents must remain intact.";
    EXPECT_EQ(source.length, 4)
        << "The source string length must remain intact.";

    CStringDestroy(&source);
}

// ============================== CStringDestroy ==============================

// Verifies that destroying a populated string safely frees its memory
// and correctly resets the struct to the null state to prevent use-after-free.
TEST(CStringTest, DestroyFreesPopulatedString) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "MemoryLeakTest"))
        << "Setup failed: could not initialize populated string.";

    CStringDestroy(&cstr);

    EXPECT_EQ(cstr.str, nullptr)
        << "Pointer must be set to null after destruction.";
    EXPECT_EQ(cstr.length, 0) << "Length must be reset to 0 after destruction.";
    EXPECT_EQ(cstr.capacity, 0)
        << "Capacity must be reset to 0 after destruction.";
}

// Verifies that destroying an empty string correctly frees the allocated buffer
// containing the null terminator and resets the struct.
TEST(CStringTest, DestroyFreesEmptyString) {
    CString cstr;
    ASSERT_TRUE(CStringInitEmpty(&cstr))
        << "Setup failed: could not initialize empty string.";

    CStringDestroy(&cstr);

    EXPECT_EQ(cstr.str, nullptr)
        << "Pointer must be set to null after destruction.";
    EXPECT_EQ(cstr.length, 0) << "Length must be reset to 0 after destruction.";
    EXPECT_EQ(cstr.capacity, 0)
        << "Capacity must be reset to 0 after destruction.";
}

// Verifies that attempting to destroy a string already in the null state
// is a safe no-op that does not cause a double-free or segmentation fault.
TEST(CStringTest, DestroyHandlesNullString) {
    CString cstr = CStringGetNull();

    CStringDestroy(&cstr);

    EXPECT_EQ(cstr.str, nullptr) << "Null string pointer must remain null.";
    EXPECT_EQ(cstr.length, 0) << "Null string length must remain 0.";
    EXPECT_EQ(cstr.capacity, 0) << "Null string capacity must remain 0.";
}

// Verifies that passing a null pointer to the destroy function safely
// returns without crashing.
TEST(CStringTest, DestroyHandlesNullPointer) {
    // If the function does not handle null pointers correctly, this will
    // segfault.
    CStringDestroy(nullptr);

    SUCCEED()
        << "CStringDestroy safely handled a null pointer without crashing.";
}

// =============================== CStringAppend ===============================

// Verifies that appending a valid C-string to an empty CString successfully
// populates it and updates the length and capacity properly.
TEST(CStringTest, AppendsToEmptyString) {
    CString cstr;
    ASSERT_TRUE(CStringInitEmpty(&cstr))
        << "Setup failed: could not initialize empty string.";

    bool result = CStringAppend(&cstr, "test");

    EXPECT_TRUE(result) << "CStringAppend must return true on success.";
    EXPECT_STREQ(cstr.str, "test")
        << "The string content must match the appended value.";
    EXPECT_EQ(cstr.length, 4)
        << "The length must equal the length of the appended string.";
    EXPECT_GE(cstr.capacity, 4)
        << "The capacity must be sufficient to hold the appended string.";

    CStringDestroy(&cstr);
}

// Verifies that appending to an already populated string accurately
// concatenates the contents and updates the metadata.
TEST(CStringTest, AppendsToPopulatedString) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Hello, "))
        << "Setup failed: could not initialize populated string.";

    bool result = CStringAppend(&cstr, "World!");

    EXPECT_TRUE(result) << "CStringAppend must return true on success.";
    EXPECT_STREQ(cstr.str, "Hello, World!")
        << "The string must contain the correctly concatenated text.";
    EXPECT_EQ(cstr.length, 13)
        << "The new length must be the sum of both string lengths.";
    EXPECT_GE(cstr.capacity, 13)
        << "The capacity must accommodate the entire concatenated string.";

    CStringDestroy(&cstr);
}

// Verifies that appending a significantly large string correctly triggers
// internal memory reallocation without corrupting the data.
TEST(CStringTest, AppendsWithReallocation) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Start: "))
        << "Setup failed: could not initialize string.";

    // Create a large string that will almost certainly exceed the initial
    // capacity.
    std::string large_append(2000, 'A');

    bool result = CStringAppend(&cstr, large_append.c_str());

    EXPECT_TRUE(result) << "CStringAppend must return true even when massive "
                           "reallocation is required.";

    std::string expected = "Start: " + large_append;
    EXPECT_STREQ(cstr.str, expected.c_str())
        << "The concatenated large string must match expected contents "
           "exactly.";
    EXPECT_EQ(cstr.length, 2007)
        << "The length must reflect the large appended string.";

    CStringDestroy(&cstr);
}

// Verifies that appending an empty string array results in a safe no-op
// that doesn't modify the string or capacity.
TEST(CStringTest, AppendsEmptyCharArray) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Invariant"))
        << "Setup failed: could not initialize string.";

    int64_t original_capacity = cstr.capacity;

    bool result = CStringAppend(&cstr, "");

    EXPECT_TRUE(result) << "Appending an empty string must succeed.";
    EXPECT_STREQ(cstr.str, "Invariant")
        << "The string contents must remain completely unchanged.";
    EXPECT_EQ(cstr.length, 9) << "The length must not change.";
    EXPECT_EQ(cstr.capacity, original_capacity)
        << "The capacity should not be altered by appending nothing.";

    CStringDestroy(&cstr);
}

// Verifies that passing a null pointer as the source string is safely rejected
// and the original string is left untouched.
TEST(CStringTest, HandlesNullSourcePointer) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Safe"))
        << "Setup failed: could not initialize string.";

    bool result = CStringAppend(&cstr, nullptr);

    EXPECT_FALSE(result)
        << "CStringAppend must return false when the source pointer is null.";
    EXPECT_STREQ(cstr.str, "Safe") << "The target string contents must not be "
                                      "modified after a failed append.";
    EXPECT_EQ(cstr.length, 4) << "The target length must not change.";

    CStringDestroy(&cstr);
}

// Verifies that appending a string to itself works correctly and does not
// corrupt memory if the internal buffer needs reallocation.
TEST(CStringTest, HandlesSelfAppend) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Echo"))
        << "Setup failed: could not initialize string.";

    bool result = CStringAppend(&cstr, cstr.str);

    EXPECT_TRUE(result) << "Self-appending must succeed.";
    EXPECT_STREQ(cstr.str, "EchoEcho")
        << "The string must be perfectly doubled without corruption.";
    EXPECT_EQ(cstr.length, 8) << "The length must be exactly doubled.";

    CStringDestroy(&cstr);
}

// =============================== CStringResize ===============================

// Verifies that resizing a string to a smaller length successfully truncates
// it, updates the length correctly, and safely inserts a new null terminator.
TEST(CStringTest, ResizeTruncatesString) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "HelloWorld"))
        << "Setup failed: could not initialize string.";

    bool result = CStringResize(&cstr, 5, 'x');

    EXPECT_TRUE(result)
        << "CStringResize must return true on successful truncation.";
    EXPECT_STREQ(cstr.str, "Hello")
        << "The string must be accurately truncated at the new length.";
    EXPECT_EQ(cstr.length, 5)
        << "The length must be updated to the truncated size.";

    CStringDestroy(&cstr);
}

// Verifies that the fill character is completely ignored when the resize
// operation truncates the string rather than expanding it.
TEST(CStringTest, ResizeIgnoresFillOnTruncation) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Hello"))
        << "Setup failed: could not initialize string.";

    bool result = CStringResize(&cstr, 2, 'z');

    EXPECT_TRUE(result) << "CStringResize must return true on success.";
    EXPECT_STREQ(cstr.str, "He")
        << "The truncated string must not contain the fill character.";
    EXPECT_EQ(cstr.length, 2)
        << "The length must equal the new specified length.";

    CStringDestroy(&cstr);
}

// Verifies that resizing to a larger length successfully expands the string,
// appends the fill character for all new spaces, and properly null-terminates.
TEST(CStringTest, ResizeExpandsString) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Hi"))
        << "Setup failed: could not initialize string.";

    bool result = CStringResize(&cstr, 5, '!');

    EXPECT_TRUE(result)
        << "CStringResize must return true on successful expansion.";
    EXPECT_STREQ(cstr.str, "Hi!!!")
        << "The string must be expanded and padded with the fill character.";
    EXPECT_EQ(cstr.length, 5) << "The length must match the new expanded size.";
    EXPECT_GE(cstr.capacity, 5)
        << "The capacity must safely hold the expanded string.";

    CStringDestroy(&cstr);
}

// Verifies that a massive resize correctly triggers memory reallocation
// and safely populates the entire new space with the fill character.
TEST(CStringTest, ResizeExpandsStringWithReallocation) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Start"))
        << "Setup failed: could not initialize string.";

    bool result = CStringResize(&cstr, 2000, 'A');

    EXPECT_TRUE(result)
        << "CStringResize must return true even during a large reallocation.";
    EXPECT_EQ(cstr.length, 2000)
        << "The length must reflect the massively expanded size.";
    EXPECT_GE(cstr.capacity, 2000)
        << "The capacity must handle the massive reallocation.";

    std::string expected = "Start" + std::string(1995, 'A');
    EXPECT_STREQ(cstr.str, expected.c_str())
        << "The entire expanded space must be filled correctly.";

    CStringDestroy(&cstr);
}

// Verifies that resizing a populated string to length 0 correctly empties it
// without necessarily destroying the underlying capacity.
TEST(CStringTest, ResizeResizesToZero) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Data"))
        << "Setup failed: could not initialize string.";

    bool result = CStringResize(&cstr, 0, 'x');

    EXPECT_TRUE(result)
        << "CStringResize must return true when shrinking to zero.";
    EXPECT_STREQ(cstr.str, "") << "The string must become completely empty.";
    EXPECT_EQ(cstr.length, 0) << "The length must be updated to 0.";

    CStringDestroy(&cstr);
}

// Verifies that passing a negative length safely rejects the operation
// and leaves the original string unmodified.
TEST(CStringTest, ResizeFailsOnNegativeLength) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Test"))
        << "Setup failed: could not initialize string.";

    bool result = CStringResize(&cstr, -1, 'x');

    EXPECT_FALSE(result)
        << "CStringResize must return false for invalid negative lengths.";
    EXPECT_STREQ(cstr.str, "Test")
        << "The string contents must remain completely unchanged.";
    EXPECT_EQ(cstr.length, 4)
        << "The string length must remain completely unchanged.";

    CStringDestroy(&cstr);
}

// Verifies that passing a null pointer safely rejects the operation
// without causing a segmentation fault.
TEST(CStringTest, ResizeHandlesNullPointer) {
    bool result = CStringResize(nullptr, 5, 'x');

    EXPECT_FALSE(result)
        << "CStringResize must return false when the string pointer is null.";
}

// =============================== CStringIsNull ===============================

// Verifies that a properly constructed null state string is accurately
// identified as being null.
TEST(CStringTest, IsNullDetectsNullState) {
    CString cstr = CStringGetNull();

    EXPECT_TRUE(CStringIsNull(&cstr))
        << "A string initialized to the null state must be identified as null.";
}

// Verifies that passing a raw null pointer safely returns true rather
// than attempting to dereference it and causing a segmentation fault.
TEST(CStringTest, IsNullDetectsNullPointer) {
    EXPECT_TRUE(CStringIsNull(nullptr))
        << "A null pointer must be identified as a null string.";
}

// Verifies that an initialized, empty string is strictly distinguished
// from a null string.
TEST(CStringTest, IsNullRejectsEmptyString) {
    CString cstr;
    ASSERT_TRUE(CStringInitEmpty(&cstr))
        << "Setup failed: could not initialize empty string.";

    EXPECT_FALSE(CStringIsNull(&cstr))
        << "An initialized empty string (allocated buffer, length 0) must not "
           "be considered null.";

    CStringDestroy(&cstr);
}

// Verifies that a standard populated string is correctly rejected as
// not being a null string.
TEST(CStringTest, IsNullRejectsPopulatedString) {
    CString cstr;
    ASSERT_TRUE(CStringInitCopyCharArray(&cstr, "Valid Data"))
        << "Setup failed: could not initialize string.";

    EXPECT_FALSE(CStringIsNull(&cstr))
        << "A populated string must not be considered null.";

    CStringDestroy(&cstr);
}
