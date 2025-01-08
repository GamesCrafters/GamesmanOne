/**
 * @file quixo_constants.h
 * @author Robert Shi (robertyishi@berkeley.edu)
 * @author GamesCrafters Research Group, UC Berkeley
 *         Supervised by Dan Garcia <ddgarcia@cs.berkeley.edu>
 * @brief Quixo helper constants.
 * @version 2.0.0
 * @date 2025-01-07
 *
 * @copyright This file is part of GAMESMAN, The Finite, Two-person
 * Perfect-Information Game Generator released under the GPL:
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMESMANONE_GAMES_QUIXO_QUIXO_CONSTANTS_H_
#define GAMESMANONE_GAMES_QUIXO_QUIXO_CONSTANTS_H_

#include <stdint.h>  // uint64_t

enum {
    kNumPlayers = 2,
    kNumVariants = 3,
    kSideLengthMax = 5,
    kBoardSizeMax = kSideLengthMax * kSideLengthMax,
    kNumLinesMax = kSideLengthMax * 2 + 2,
    kNumMovesPerDirMax = kSideLengthMax * 3 - 4,
};

extern const int kNumLines[kNumVariants];
extern const int kNumMovesPerDir[kNumVariants];

extern const uint64_t kLines[kNumVariants][kNumPlayers][kNumLinesMax];
extern const uint64_t kEdges[kNumVariants][kNumPlayers];
extern const uint64_t kMoveLeft[kNumVariants][kNumMovesPerDirMax][4];
extern const uint64_t kMoveRight[kNumVariants][kNumMovesPerDirMax][4];
extern const uint64_t kMoveUp[kNumVariants][kNumMovesPerDirMax][4];
extern const uint64_t kMoveDown[kNumVariants][kNumMovesPerDirMax][4];
extern const int kDirIndexToSrc[kNumVariants][4][kNumMovesPerDirMax];
extern const int kDirSrcToIndex[kNumVariants][4][kBoardSizeMax];

#endif  // GAMESMANONE_GAMES_QUIXO_QUIXO_CONSTANTS_H_
