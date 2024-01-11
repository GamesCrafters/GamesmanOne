#ifndef GAMESMANONE_CORE_GAMESMAN_HEADLESS_H_
#define GAMESMANONE_CORE_GAMESMAN_HEADLESS_H_

/** @brief Enumeration of all possible actions in headless mode. */
enum HeadlessAction {
    kInvalidHeadlessAction = -1,
    kHeadlessSolve,
    kHeadlessAnalyze,
    kHeadlessQuery,
    kHeadlessGetStart,
    kHeadlessGetRandom,
    kNumHeadlessActions,
};

int GamesmanHeadlessMain(int argc, char **argv);

#endif  // GAMESMANONE_CORE_GAMESMAN_HEADLESS_H_
