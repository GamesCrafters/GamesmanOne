#ifndef GAMESMANONE_GAMES_MILLS_VARIANTS_H_
#define GAMESMANONE_GAMES_MILLS_VARIANTS_H_

#include <stdint.h>  // int8_t

#include "core/types/gamesman_types.h"

// clang-format off
/*
Option 1: board and pieces
    a) 5 pieces each on a 16-slot board (5mm)
    b) 6 pieces each on a 16-slot board (6mm)
    c) 7 pieces each on a 17-slot board (7mm)
    d) 9 pieces each on a 24-slot board (9mm)
    e) 10 pieces each on a 24-slot board (10mm)
    f) 11 pieces each on a 24-slot board with diagonals (11mm)
    g) 12 pieces each on a 24-slot board with diagonals (Morabaraba/12mm)
    h) 12 pieces each on a 25-slot Sesotho board (Sesotho Morabaraba)

Option 2: flying rule
    a) not allowed
    b) allowed when left with <= 3 pieces
    c) allowed when left with <= 4 pieces
    d) allowed at all times

Option 3: Lasker rule (merge placement and moving phases)
    a) no
    b) yes, flying rule applies to total pieces remaining
    c) yes, flying rule applies to pieces on board

Option 4: removal rule
    a) standard: pieces in a mill can only be removed when all pieces are in a
       mill
    b) strict: if all pieces are in a mill, no piece is removed
    c) lenient: any piece may be removed

Option 5: Misère (flip winning and losing conditions)
    a) false
    b) true

Standard combinations of options:
    (a) Five Men's Morris: [aaaaa] (currently variant 0)
    (b) Six Men's Morris: [baaaa] (currently variant 72)
    (c) Seven Men's Morris: [caaaa] (currently variant 144)
    (d) Nine Men's Morris: [dbaaa], [dbaba] (currently variant 234 and 236)
    (e) Lasker Morris (on board): [ebcaa], [ebcba] (currently variant 318 and 320)
    (f) Lasker Morris (remaining): [ebbaa], [ebbba] (currently variant 312 and 314)
    (g) Eleven Men's Morris: [fcaaa] (currently variant 396)
    (h) Twelve Men's Morris/Morabaraba: [gaaaa] (currently variant 450)
    (i) Sesotho Morabaraba: [haaaa] (currently variant 522)
*/
// clang-format on

static const char *const kMillsBoardAndPiecesChoices[] = {
    "5 pieces each on a 16-slot board (Five Men's Morris)",
    "6 pieces each on a 16-slot board (Six Men's Morris)",
    "7 pieces each on a 17-slot board (Seven Men's Morris)",
    "9 pieces each on a 24-slot board (Nine Men's Morris)",
    "10 pieces each on a 24-slot board (Lasker Morris)",
    "11 pieces each on a 24-slot board with diagonals (Eleven Men's Morris)",
    "12 pieces each on a 24-slot board with diagonals (Morabaraba/Twelve Men's "
    "Morris)",
    "12 pieces each on a 25-slot Sesotho board (Sesotho Morabaraba)",
};
#define NUM_BOARD_AND_PIECES_CHOICES       \
    (sizeof(kMillsBoardAndPiecesChoices) / \
     sizeof(kMillsBoardAndPiecesChoices[0]))

static const int8_t kPiecesPerPlayer[NUM_BOARD_AND_PIECES_CHOICES] = {
    5, 6, 7, 9, 10, 11, 12, 12,
};

static const char *const kMillsFlyingRuleChoices[] = {
    "Not allowed",
    "Allowed with fewer than 3 pieces on board",
    "Allowed with fewer than 4 pieces on board",
    "Allowed always",
};
#define NUM_FLYING_RULE_CHOICES \
    (sizeof(kMillsFlyingRuleChoices) / sizeof(kMillsFlyingRuleChoices[0]))

static const char *const kMillsLaskerRuleChoices[] = {
    "No",
    "Yes, flying rule applies to total pieces remaining",
    "Yes, flying rule applies to pieces on board",
};
#define NUM_LASKER_RULE_CHOICES \
    (sizeof(kMillsLaskerRuleChoices) / sizeof(kMillsLaskerRuleChoices[0]))

static const char *const kMillsRemovalRuleChoices[] = {
    "Standard",
    "Strict",
    "Lenient",
};
#define NUM_REMOVAL_RULE_CHOICES \
    (sizeof(kMillsRemovalRuleChoices) / sizeof(kMillsRemovalRuleChoices[0]))

static const char *const kBooleanChoices[] = {
    "False",
    "True",
};

static const GameVariantOption mills_variant_options[6] = {
    {
        .name = "Board and Pieces",
        .num_choices = NUM_BOARD_AND_PIECES_CHOICES,
        .choices = kMillsBoardAndPiecesChoices,
    },
    {
        .name = "Flying Rule",
        .num_choices = NUM_FLYING_RULE_CHOICES,
        .choices = kMillsFlyingRuleChoices,
    },
    {
        .name = "Lasker Rule",
        .num_choices = NUM_LASKER_RULE_CHOICES,
        .choices = kMillsLaskerRuleChoices,
    },
    {
        .name = "Removal Rule",
        .num_choices = NUM_REMOVAL_RULE_CHOICES,
        .choices = kMillsRemovalRuleChoices,
    },
    {
        .name = "Misere",
        .num_choices = 2,
        .choices = kBooleanChoices,
    },
};

#endif  // GAMESMANONE_GAMES_MILLS_VARIANTS_H_
