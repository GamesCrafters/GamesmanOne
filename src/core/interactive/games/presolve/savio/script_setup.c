#include "core/interactive/games/presolve/savio/script_setup.h"

#include <ctype.h>    // isalnum, isdigit
#include <stdbool.h>  // bool, true, false
#include <stdio.h>    // sprintf, printf
#include <stdlib.h>   // atoi
#include <string.h>   // strlen

#include "core/constants.h"
#include "core/interactive/automenu.h"
#include "core/interactive/games/presolve/match.h"
#include "core/misc.h"
#include "core/savio/savio.h"
#include "core/savio/scriptgen.h"
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
static ReadOnlyString items_p[NUM_ITEMS];
static char keys[NUM_ITEMS][kKeyLengthMax + 1];
static ReadOnlyString keys_p[NUM_ITEMS];
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
static void ReplaceSpecialCharacters(char *str);
static bool IsValidTimeLimit(ReadOnlyString str);

int InteractiveSavioScriptSetup(ReadOnlyString key) {
    InitGlobalVariables(key);
    static ConstantReadOnlyString title = "Adjust Savio Settings";
    UpdateItems();
    InitKeys();
    InitHooks();

    return AutoMenu(title, NUM_ITEMS, items_p, keys_p, hooks, &UpdateItems);
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
    settings.num_nodes = IntMin2(kSavioNumNodesMax, partition->num_nodes);
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

    for (int i = 0; i < NUM_ITEMS; ++i) {
        items_p[i] = (ReadOnlyString)&items[i];
    }
}

static void InitKeys(void) {
    sprintf(keys[0], "c");
    keys_p[0] = (ReadOnlyString)&keys[0];
    for (int i = 1; i < NUM_ITEMS; ++i) {
        sprintf(keys[i], "%d", i);
        keys_p[i] = (ReadOnlyString)&keys[i];
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
    (void)key;  // Unused
    SavioScriptGeneratorWrite(&settings);

    return 2;  // Go back 2 levels in menu.
}

static int PromptForJobName(ReadOnlyString key) {
    (void)key;  // Unused
    char buf[kSavioJobNameLengthMax + 1];
    printf(
        "Setting the displayed name of your task in SLURM's job list. "
        "Only alpha-numeric characters and underscores are allowed. All other "
        "characters will be replaced with underscores.\n\n"
        "Please enter the job name (%d or fewer characters) or leave blank "
        "to discard changes and go back to the previous menu",
        kSavioJobNameLengthMax);
    PromptForInput("", buf, kSavioJobNameLengthMax);
    if (strlen(buf) == 0) return 0;

    ReplaceSpecialCharacters(buf);
    strcpy(settings.job_name, buf);

    return 0;
}

static int PromptForAccount(ReadOnlyString key) {
    (void)key;  // Unused
    char buf[kSavioAccountNameLengthMax + 1];
    printf(
        "Setting the name of the Savio account that will be charged for "
        "service units. It is unlikely that you will need to change the "
        "default value if you are a member of the GamesCrafters project "
        "(fc_gamecrafters).\n\n"
        "Please enter the account name (%d or fewer characters) or leave blank "
        "to discard changes and go back to the previous menu",
        kSavioAccountNameLengthMax);
    PromptForInput("", buf, kSavioAccountNameLengthMax);
    if (strlen(buf) == 0) return 0;

    ReplaceSpecialCharacters(buf);
    strcpy(settings.account, buf);

    return 0;
}

static int PromptForNumNodes(ReadOnlyString key) {
    (void)key;  // Unused
    char buf[kInt32Base10StringLengthMax + 1];
    int nodes_min = 1;
    int nodes_max = game->solver->supports_mpi ? kSavioNumNodesMax : 1;
    printf(
        "Number of compute nodes to use in the chosen partition (%s). Savio "
        "enforces that a maximum of %d compute nodes can be allocated per "
        "job. Since non-tier games are solved on the same node, GAMESMAN will "
        "limit the number of nodes to 1 if the current game is not a tier "
        "game.\n\n"
        "Please enter the number of nodes to use for this job (%d-%d) or 'b' "
        "to discard changes and return to the previous menu%s",
        partition->name, kSavioNumNodesMax, nodes_min, nodes_max, "");
    PromptForInput("", buf, kInt32Base10StringLengthMax);
    if (strcmp(buf, "b") == 0) return 0;
    int parsed = atoi(buf);
    while (parsed < nodes_min || parsed > nodes_max) {
        printf(
            "\nSorry, the number you entered (%d) is outside the range of "
            "valid number of nodes (%d-%d). Please try again or enter 'b' to "
            "discard changes and return to the previous menu",
            parsed, nodes_min, nodes_max);
        PromptForInput("", buf, kInt32Base10StringLengthMax);
        if (strcmp(buf, "b") == 0) return 0;
        parsed = atoi(buf);
    }
    settings.num_nodes = parsed;

    return 0;
}

static int PromptForNumTasksPerNode(ReadOnlyString key) {
    (void)key;  // Unused
    char buf[kInt32Base10StringLengthMax + 1];
    int tasks_min = 1;
    int tasks_max = partition->num_cpu;
    printf(
        "Setting the number of tasks to allocate on each compute node. "
        "Changing this value will also adjust the number of CPUs per task "
        "according to the available number of CPUs on the compute nodes in the "
        "chosen partition to max out the number of CPUs utilized on each node. "
        "To utilize all CPUs on each node, set this number to a factor of %d - "
        "the number of CPUs available on each node in the chosen partition "
        "(%s).\n\n"
        "Please enter the number of tasks to run on each node (%d-%d) or 'b' "
        "to discard changes and return to the previous menu",
        partition->num_cpu, partition->name, tasks_min, tasks_max);
    PromptForInput("", buf, kInt32Base10StringLengthMax);
    if (strcmp(buf, "b") == 0) return 0;
    int parsed = atoi(buf);
    while (parsed < tasks_min || parsed > tasks_max) {
        printf(
            "\nSorry, the number you entered (%d) is outside the range of "
            "valid number of tasks per node (%d-%d). Please try again or enter "
            "'b' to discard changes and return to the previous menu",
            parsed, tasks_min, tasks_max);
        PromptForInput("", buf, kInt32Base10StringLengthMax);
        if (strcmp(buf, "b") == 0) return 0;
        parsed = atoi(buf);
    }
    settings.ntasks_per_node = parsed;

    return 0;
}

static int PromptForNumCpuPerTask(ReadOnlyString key) {
    (void)key;  // Unused
    char buf[kInt32Base10StringLengthMax + 1];
    int cpus_min = 1;
    int cpus_max = partition->num_cpu;
    printf(
        "Setting the number of CPUs to use on each task. Changing this value "
        "also adjusts the number of tasks allocated per compute node according "
        "to the number of CPUs availale on each node in the chosen partition "
        "to max out the number of CPUs utilized on each node. To utilize all "
        "CPUs on each node, set this number to a factor of %d - the number of "
        "CPUs available on each node in the chosen partition (%s).\n\n"
        "Please enter the number of tasks to run on each node (%d-%d) or 'b' "
        "to discard changes and return to the previous menu",
        partition->num_cpu, partition->name, cpus_min, cpus_max);
    PromptForInput("", buf, kInt32Base10StringLengthMax);
    if (strcmp(buf, "b") == 0) return 0;
    int parsed = atoi(buf);
    while (parsed < cpus_min || parsed > cpus_max) {
        printf(
            "\nSorry, the number you entered (%d) is outside the range of "
            "valid number of CPUs per task (%d-%d). Please try again or enter "
            "'b' to discard changes and return to the previous menu",
            parsed, cpus_min, cpus_max);
        PromptForInput("", buf, kInt32Base10StringLengthMax);
        if (strcmp(buf, "b") == 0) return 0;
        parsed = atoi(buf);
    }
    settings.ntasks_per_node =
        SavioGetNumTasksPerNode(partition->num_cpu, parsed);

    return 0;
}

static int PromptForTimeLimit(ReadOnlyString key) {
    (void)key;  // Unused
    char buf[kSavioTimeLimitLengthMax + 1];
    printf(
        "Setting a new time limit for the job. The hard time limit for "
        "regular jobs on Savio is 72:00:00 or 3 days. Regular jobs that run "
        "for longer than 3 days will be terminated. If your job finishes "
        "before it reaches this time limit, it will terminate and your account "
        "will only be charged for the amount of time actually used. If your "
        "job reaches the time limit, it will be killed regardless of its "
        "status. It is okay to always set this to the 3-day maximum. "
        "However, the SLURM job scheduler may schedule jobs that have a "
        "shorter time limit to run first. This might be helpful if you want to "
        "run a quick job when there are a lot of jobs in the SLURM queue.\n\n"
        "The time limit must be of format \"hh:mm:ss\" where mm and ss must be "
        "values between 00 and 59, and the total time must not be longer than "
        "72 hours.\n\n"
        "Please enter a new time limit for the job, or enter 'b' to discard "
        "changes and return to the previous menu");
    PromptForInput("", buf, kSavioTimeLimitLengthMax);
    while (!IsValidTimeLimit(buf)) {
        printf(
            "\nSorry, the time limit you entered (%s) is outside of the valid "
            "range of time limits (00:00:00 - 72:00:00) or not of the valid "
            "time format \"hh:mm:ss\". Please try again or enter 'b' to "
            "discard changes and return to the previous menu",
            buf);
        PromptForInput("", buf, kSavioTimeLimitLengthMax);
    }
    strcpy(settings.time_limit, buf);

    return 0;
}

static int IntMin2(int x, int y) { return x < y ? x : y; }

static void ReplaceSpecialCharacters(char *str) {
    // Loop through each character in the string
    for (int i = 0; str[i] != '\0' && i < 31; i++) {
        // Check if the character is not a letter or digit
        if (!isalnum((unsigned char)str[i])) {
            // Replace with underscore
            str[i] = '_';
        }
    }
}

static bool IsValidTimeLimit(ReadOnlyString str) {
    // Check if the string length is exactly 8 characters (for "hh:mm:ss")
    if (strlen(str) != 8) return false;

    // Check for correct format "hh:mm:ss"
    if (str[2] != ':' || str[5] != ':') return false;

    // Check if hh, mm, ss are all digits
    for (int i = 0; i < 8; i++) {
        if (i == 2 || i == 5) continue;  // skip ':' characters
        if (!isdigit((unsigned char)str[i])) return false;
    }

    // Extract hours, minutes, and seconds
    int hh, mm, ss;
    hh = (str[0] - '0') * 10 + (str[1] - '0');
    mm = (str[3] - '0') * 10 + (str[4] - '0');
    ss = (str[6] - '0') * 10 + (str[7] - '0');

    // Check if hh is between 00 and 72, and mm, ss are between 00 and 59
    if (hh < 0 || hh > 72 || mm < 0 || mm > 59 || ss < 0 || ss > 59) {
        return false;
    }

    // Check if total time exceeds 72 hours
    if (hh == 72 && (mm > 0 || ss > 0)) return false;

    return true;
}
