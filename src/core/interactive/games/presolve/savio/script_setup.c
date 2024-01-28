#include "core/interactive/games/presolve/savio/script_setup.h"

#include <stdio.h>   // sprintf
#include <stdlib.h>  // atoi

#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/savio/savio.h"
#include "core/types/gamesman_types.h"

static SavioJobSettings settings;
static int partition_id;
static const SavioPartition *partition;
static const Game *game;
static int variant_id;

#define NUM_ITEMS 7
// Note that the size of each string here must be longer than the maximum
// length that an item can grow into.
static char items[NUM_ITEMS][2 * kSavioJobNameLengthMax + 1];
static char keys[NUM_ITEMS][kKeyLengthMax + 1];
static HookFunctionPointer hooks[NUM_ITEMS];

static void InitGlobalVariables(ReadOnlyString key);
static void RestoreDefaultSettings(void);

static void UpdateItems(void);
static void InitKeys(void);
static void InitHooks(void);

static int ConfirmSettings(ReadOnlyString key);
static int PromptForJobName(ReadOnlyString key);
static int PromptForAccount(ReadOnlyString key);
static int PromptForNumNodes(ReadOnlyString key);
static int PromptForNumTasksPerNode(ReadOnlyString key);
static int PromptForNumCpuPerTask(ReadOnlyString key);
static int PromptForTimeLimit(ReadOnlyString key);

static int IntMin2(int x, int y);

int InteractiveSavioScriptSetup(ReadOnlyString key) {
    InitGlobalVariables(key);
    static ConstantReadOnlyString title = "Adjust Savio Settings";
    UpdateItems();
    InitKeys();
    InitHooks();

    return AutoMenu(title, NUM_ITEMS, items, keys, hooks, &UpdateItems);
}

static void InitGlobalVariables(ReadOnlyString key) {
    partition_id = atoi(key);
    partition = &kSavioPartitions[partition_id];
    game = InteractiveMatchGetCurrentGame();
    variant_id = InteractiveMatchGetVariantIndex();
    RestoreDefaultSettings();
}

static void RestoreDefaultSettings(void) {
    sprintf(settings.game_name, "%s", game->name);
    settings.game_variant_id = variant_id;
    sprintf(settings.job_name, "%s", game->name);
    sprintf(settings.account, "%s", kSavioDefaultAccount);
    settings.partition_id = partition_id;
    // Change game interface and specify tier vs regular
    settings.num_nodes = IntMin2(kSavioTierNumNodesMax, partition->num_nodes);
    settings.ntasks_per_node = kSavioDefaultNumTasksPerNode;
    sprintf(settings.time_limit, "%s", kSavioDefaultTimeLimit);
}

static void UpdateItems(void) {
    sprintf(items[0], "Confirm");
    sprintf(items[1], "Job name: [%s]", settings.job_name);
    sprintf(items[2], "Savio account: [%s]", settings.account);
    sprintf(items[3], "Number of nodes to use: [%d]", settings.num_nodes);
    sprintf(items[4], "Number of tasks per node: [%d]",
            settings.ntasks_per_node);
    sprintf(
        items[5], "Number of CPUs per task: [%d]",
        SavioGetNumCpuPerTask(partition->num_cpu, settings.ntasks_per_node));
    sprintf(items[6], "Time limit: [%s]", settings.time_limit);
}

static void InitKeys(void) {
    sprintf(keys[0], "c");
    for (int i = 1; i < NUM_ITEMS; ++i) {
        sprintf(keys[i], "%d", i);
    }
}

static void InitHooks(void) {
    hooks[0] = &ConfirmSettings;
    hooks[1] = &PromptForJobName;
    hooks[2] = &PromptForAccount;
    hooks[3] = &PromptForNumNodes;
    hooks[4] = &PromptForNumTasksPerNode;
    hooks[5] = &PromptForNumCpuPerTask;
    hooks[6] = &PromptForTimeLimit;
}

static int ConfirmSettings(ReadOnlyString key) {
    // TODO
    (void)key;
}

static int PromptForJobName(ReadOnlyString key) {
    // TODO
}

static int PromptForAccount(ReadOnlyString key) {
    // TODO
}

static int PromptForNumNodes(ReadOnlyString key) {
    // TODO
}

static int PromptForNumTasksPerNode(ReadOnlyString key) {
    // TODO
}

static int PromptForNumCpuPerTask(ReadOnlyString key) {
    // TODO
}

static int PromptForTimeLimit(ReadOnlyString key) {
    // TODO
}

static int IntMin2(int x, int y) { return x < y ? x : y; }
