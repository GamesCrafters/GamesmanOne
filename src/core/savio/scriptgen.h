#ifndef GAMESMANONE_CORE_SAVIO_SCRIPTGEN_H_
#define GAMESMANONE_CORE_SAVIO_SCRIPTGEN_H_

#include "core/types/gamesman_types.h"

enum SavioScriptConstants {
    kJobNameLengthMax = 31,
    kAccountNameLengthMax = 31,

    // Time limit must be of the format "hh:mm:ss".
    kTimeLimitLengthMax = 8,  
};

int SavioScriptGeneratorGenerate(void);

#endif  // GAMESMANONE_CORE_SAVIO_SCRIPTGEN_H_
