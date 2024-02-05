#include "core/savio/scriptgen.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdio.h>    // FILE, printf

#include "core/misc.h"
#include "core/savio/savio.h"
#include "core/types/gamesman_types.h"

static int PrintSbatch(FILE *file);
static int PrintStringOption(FILE *file, ReadOnlyString option_name,
                             ReadOnlyString value);
static int PrintIntegerOption(FILE *file, ReadOnlyString option_name,
                              int value);
static int PrintJobName(FILE *file, ReadOnlyString job_name);
static int PrintAccount(FILE *file, ReadOnlyString account);
static int PrintPartition(FILE *file, ReadOnlyString partition);
static int PrintNodes(FILE *file, int num_nodes);
static int PrintNtasksPerNode(FILE *file, int num_tasks_per_node);
static int PrintCpusPerTask(FILE *file, int cpus_per_task);
static int PrintTime(FILE *file, ReadOnlyString time_limit);
static int PrintExclusive(FILE *file);
static int Print512GbSavio4Request(FILE *file);
static int PrintModuleLoad(FILE *file, bool use_savio4);
static int PrintExportOmpNumThreads(FILE *file);
static int PrintSolveCommand(FILE *file, ReadOnlyString game_name,
                             int variant_id);

// -----------------------------------------------------------------------------

int SavioScriptGeneratorWrite(const SavioJobSettings *settings) {
    static const char extension[] = ".sh";
    char file_name[kSavioJobNameLengthMax + sizeof(extension) + 1];
    sprintf(file_name, "%s%s", settings->job_name, extension);
    FILE *file = GuardedFopen(file_name, "w");
    if (file == NULL) return kFileSystemError;

    const SavioPartition *partition = &kSavioPartitions[settings->partition_id];
    PrintJobName(file, settings->job_name);
    PrintAccount(file, settings->account);
    PrintPartition(file, partition->name);
    PrintNodes(file, settings->num_nodes);
    PrintNtasksPerNode(file, settings->ntasks_per_node);
    PrintCpusPerTask(file, partition->num_cpu / settings->ntasks_per_node);
    PrintTime(file, settings->time_limit);
    if (!partition->per_node_allocation) PrintExclusive(file);
    if (settings->partition_id == kSavio4_htc_512gb)
        Print512GbSavio4Request(file);
    bool use_savio4 = settings->partition_id == kSavio4_htc_256gb ||
                      settings->partition_id == kSavio4_htc_512gb;
    PrintModuleLoad(file, use_savio4);
    PrintExportOmpNumThreads(file);
    PrintSolveCommand(file, settings->game_name, settings->game_variant_id);

    int error = GuardedFclose(file);
    if (error) return kFileSystemError;

    printf(
        "Successfully written to %s\nRun \"sbatch %s\" in a different terminal "
        "to submit the job.\n\n",
        file_name, file_name);

    return kNoError;
}

// -----------------------------------------------------------------------------

static int PrintSbatch(FILE *file) { return fprintf(file, "#SBATCH "); }

static int PrintStringOption(FILE *file, ReadOnlyString option_name,
                             ReadOnlyString value) {
    int count1 = PrintSbatch(file);
    if (count1 < 0) return count1;

    int count2;
    if (value != NULL) {
        count2 = fprintf(file, "--%s=%s\n", option_name, value);
    } else {
        count2 = fprintf(file, "--%s\n", option_name);
    }
    if (count2 < 0) return count2;

    return count1 + count2;
}

static int PrintIntegerOption(FILE *file, ReadOnlyString option_name,
                              int value) {
    int count1 = PrintSbatch(file);
    if (count1 < 0) return count1;

    int count2 = fprintf(file, "--%s=%d\n", option_name, value);
    if (count2 < 0) return count2;

    return count1 + count2;
}

static int PrintJobName(FILE *file, ReadOnlyString job_name) {
    return PrintStringOption(file, "job-name", job_name);
}

static int PrintAccount(FILE *file, ReadOnlyString account) {
    return PrintStringOption(file, "account", account);
}

static int PrintPartition(FILE *file, ReadOnlyString partition) {
    return PrintStringOption(file, "partition", partition);
}

static int PrintNodes(FILE *file, int num_nodes) {
    return PrintIntegerOption(file, "nodes", num_nodes);
}

static int PrintNtasksPerNode(FILE *file, int num_tasks_per_node) {
    return PrintIntegerOption(file, "ntasks-per-node", num_tasks_per_node);
}

static int PrintCpusPerTask(FILE *file, int cpus_per_task) {
    return PrintIntegerOption(file, "cpus-per-task", cpus_per_task);
}

static int PrintTime(FILE *file, ReadOnlyString time_limit) {
    return PrintStringOption(file, "time", time_limit);
}

static int PrintExclusive(FILE *file) {
    return PrintStringOption(file, "exclusive", NULL);
}

static int Print512GbSavio4Request(FILE *file) {
    return fprintf(file,
                   "\n# Explicitly request a 512 GB memory savio4_htc node\n"
                   "#SBATCH -C savio4_m512\n");
}

static int PrintModuleLoad(FILE *file, bool use_savio4) {
    if (use_savio4) {
        return fprintf(
            file,
            "\n# 2024-01-26: Current savio4_htc configuration requires "
            "using the following non-default modules for best performance\n"
            "module load gcc/11.3.0 ucx/1.14.0 openmpi/5.0.0-ucx\n");
    }
    return fprintf(file, "module load gcc openmpi\n");
}

static int PrintExportOmpNumThreads(FILE *file) {
    return fprintf(file, "export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK\n");
}

static int PrintSolveCommand(FILE *file, ReadOnlyString game_name,
                             int variant_id) {
    return fprintf(file, "mpirun bin/gamesman solve %s %d\n", game_name,
                   variant_id);
}
