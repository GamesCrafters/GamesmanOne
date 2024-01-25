#include "core/savio/scriptgen.h"

#include <stdbool.h>  // bool
#include <stddef.h>   // NULL
#include <stdio.h>    // FILE

#include "core/misc.h"
#include "core/types/gamesman_types.h"

typedef struct SavioPartition {
    ConstantReadOnlyString name;
    int nodes;
    int num_cpu;
    int mem_gb;
    bool per_node;
} SavioPartition;

enum SavioPartitions {
    kSavio3_16c32t,
    kSavio3_20c40t,
    kSavio3_htc,
    kSavio4_htc_256gb,
    kSavio4_htc_512gb,
    kNumSavioPartitions,
};

static const SavioPartition kSavioPartitions[kNumSavioPartitions] = {
    {.name = "savio3",
     .nodes = 112,
     .num_cpu = 32,
     .mem_gb = 96,
     .per_node = true},

    {.name = "savio3",
     .nodes = 80,
     .num_cpu = 40,
     .mem_gb = 96,
     .per_node = true},

    {.name = "savio3_htc",
     .nodes = 24,
     .num_cpu = 40,
     .mem_gb = 384,
     .per_node = false},

    {.name = "savio4_htc",
     .nodes = 24,
     .num_cpu = 56,
     .mem_gb = 512,
     .per_node = false},

    {.name = "savio4_htc",
     .nodes = 84,
     .num_cpu = 56,
     .mem_gb = 256,
     .per_node = false},
};

ConstantReadOnlyString kDefaultAccount = "fc_gamecrafters";

const int kDefaultPartition = kSavio4_htc_256gb;
const int kMaxRegularNumNodes = 1;
const int kMaxTierNumNodes = 24;
const int kDefaultTasksPerNode = 1;
ConstantReadOnlyString kDefaultTimeLimit = "72:00:00";

typedef struct SavioSolverScript {
    char game_name[kGameNameLengthMax + 1];
    int game_variant_id;
    char job_name[kJobNameLengthMax + 1];
    char account[kAccountNameLengthMax + 1];
    int partition;
    int num_nodes;
    int ntasks_per_node;
    char time_limit[kTimeLimitLengthMax + 1];
} SavioSolverScript;

int SavioScriptGeneratorGenerate(void) {
    /* Parameters */
    ReadOnlyString game_name;
}

static int SavioScriptGeneratorWrite(const SavioSolverScript *script) {
    static const char extension[] = ".sh";
    char file_name[kJobNameLengthMax + sizeof(extension) + 1];
    sprintf(file_name, "%s%s", script->job_name, extension);
    FILE *file = GuardedFopen(file_name, "w");
    if (file == NULL) return kFileSystemError;

    const SavioPartition *partition = &kSavioPartitions[script->partition];
    PrintJobName(file, script->job_name);
    PrintAccount(file, script->account);
    PrintPartition(file, partition->name);
    PrintNodes(file, script->num_nodes);
    PrintNtasksPerNode(file, script->ntasks_per_node);
    PrintCpusPerTask(file, partition->num_cpu / script->ntasks_per_node);
    PrintTime(file, script->time_limit);
    if (!partition->per_node) PrintExclusive(file);
    if (script->partition == kSavio4_htc_512gb) Print512GbSavio4Request(file);
    bool use_savio4 = script->partition == kSavio4_htc_256gb ||
                      script->partition == kSavio4_htc_512gb;
    PrintModuleLoad(file, use_savio4);
    PrintExportOmpNumThreads(file);
    PrintSolveCommand(file, script->game_name, script->game_variant_id);

    int error = GuardedFclose(file);
    if (error) return kFileSystemError;

    return kNoError;
}

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
    return fprintf(file, "#SBATCH -C savio4_m512\n");
}

static int PrintModuleLoad(FILE *file, bool use_savio4) {
    if (use_savio4) {
        return fprintf(file,
                       "module load gcc/11.3.0 ucx/1.14.0 openmpi/5.0.0-ucx\n");
    }
    return fprintf(file, "module load gcc openmpi\n");
}

static int PrintExportOmpNumThreads(FILE *file) {
    return fprintf(file, "export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK\n");
}

static int PrintSolveCommand(FILE *file, ReadOnlyString game_name,
                             int variant_id) {
    return fprintf(file, "mpirun bin/gamesman mpisolve %s %d\n", game_name,
                   variant_id);
}

/*
#!/bin/bash
#SBATCH --job-name=<job_name>
#SBATCH --account=<account_name>
#SBATCH --partition=<partition_name>
#SBATCH --nodes=<num_nodes>
#SBATCH --ntasks-per-node=<ntasks_per_node>
#SBATCH --cpus-per-task=<num_cpu / ntasks_per_node>
#SBATCH --time=<time_limit>

# To request for all memory on a per-core node
#SBATCH --exclusive

# To request 512GB savio4_htc
#SBATCH -C savio4_m512

# If using savio4_htc, load ucx for optimal MPI performance
module load gcc/11.3.0 ucx/1.14.0 openmpi/5.0.0-ucx

# On other nodes, use the following
# module load gcc openmpi

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
mpirun bin/gamesman mpisolve <game_name> <variant_id>
*/
