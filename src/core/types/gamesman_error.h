#ifndef GAMESMANONE_CORE_GAMESMAN_ERROR_H_
#define GAMESMANONE_CORE_GAMESMAN_ERROR_H_

enum GamesmanError {
    kNoError = 0,  // No error should always be 0.
    kMallocFailureError,
    kNotImplementedError,
    kNotReachedError,
    kIntegerOverflowError,
    kMemoryOverflowError,
    kFileSystemError,
    kIllegalArgumentError,
    kIllegalGameNameError,
    kIllegalGameVariantError,
    kIllegalGamePositionError,
    kIllegalGamePositionValueError,
    kIllegalGameTierGraphError,
    kIllegalSolverOptionError,
    kUseBeforeInitializationError,
    kRuntimeError,
};

#endif  // GAMESMANONE_CORE_GAMESMAN_ERROR_H_
