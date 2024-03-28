#include "core/savio/scriptgen.h"

#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // NULL
#include <stdio.h>    // FILE, printf, sprintf

#include "core/misc.h"
#include "core/savio/savio.h"
#include "core/types/gamesman_types.h"

static int GetOmpNumThreads(int cpus_per_node, int ntasks_per_node,
                            bool bind_to_cores);
static int GetNumProcesses(int ntasks_per_node, int num_nodes);
static bool IsUsingSavio4(int partition_id);

static int PrintSbatch(FILE *file);
static int PrintStringOption(FILE *file, ReadOnlyString option_name,
                             ReadOnlyString value);
static int PrintIntegerOption(FILE *file, ReadOnlyString option_name,
                              int value);
static int PrintShebang(FILE *file);
static int PrintJobName(FILE *file, ReadOnlyString job_name);
static int PrintAccount(FILE *file, ReadOnlyString account);
static int PrintPartition(FILE *file, ReadOnlyString partition);
static int PrintNodes(FILE *file, int num_nodes);
static int PrintTime(FILE *file, ReadOnlyString time_limit);
static int PrintExclusive(FILE *file);
static int Print512GbSavio4Request(FILE *file);
static int PrintModuleLoad(FILE *file, bool use_savio4);
static int PrintExportOmpNumThreads(FILE *file, int num_threads);
static int PrintSolveCommand(FILE *file, int num_processes,
                             int cpus_per_proccess, ReadOnlyString game_name,
                             int variant_id);

// -----------------------------------------------------------------------------

int SavioScriptGeneratorWrite(const SavioJobSettings *settings) {
    static const char extension[] = ".sh";
    char file_name[kSavioJobNameLengthMax + sizeof(extension) + 1];
    sprintf(file_name, "%s%s", settings->job_name, extension);
    FILE *file = GuardedFopen(file_name, "w");
    if (file == NULL) return kFileSystemError;

    const SavioPartition *partition = &kSavioPartitions[settings->partition_id];
    int omp_num_threads =
        GetOmpNumThreads(partition->num_cpu, settings->ntasks_per_node,
                         settings->bind_omp_threads_to_cores);
    int num_proccesses =
        GetNumProcesses(settings->ntasks_per_node, settings->num_nodes);
    int cpus_per_process =
        SavioGetNumCpuPerTask(partition->num_cpu, settings->ntasks_per_node);
    bool use_savio4 = IsUsingSavio4(settings->partition_id);

    PrintShebang(file);
    PrintJobName(file, settings->job_name);
    PrintAccount(file, settings->account);
    PrintPartition(file, partition->name);
    PrintNodes(file, settings->num_nodes);
    PrintTime(file, settings->time_limit);
    if (!partition->per_node_allocation) PrintExclusive(file);
    if (settings->partition_id == kSavio4_htc_512gb) {
        Print512GbSavio4Request(file);
    }
    PrintModuleLoad(file, use_savio4);
    PrintExportOmpNumThreads(file, omp_num_threads);
    PrintSolveCommand(file, num_proccesses, cpus_per_process,
                      settings->game_name, settings->game_variant_id);

    int error = GuardedFclose(file);
    if (error) return kFileSystemError;

    printf(
        "Successfully written to %s\nRun \"sbatch %s\" in a different terminal "
        "to submit the job.\n\n",
        file_name, file_name);

    return kNoError;
}

// -----------------------------------------------------------------------------

static int GetOmpNumThreads(int cpus_per_node, int ntasks_per_node,
                            bool bind_to_cores) {
    return cpus_per_node / ntasks_per_node / (bind_to_cores + 1);
}

static int GetNumProcesses(int ntasks_per_node, int num_nodes) {
    return ntasks_per_node * num_nodes;
}

static bool IsUsingSavio4(int partition_id) {
    if (partition_id == kSavio4_htc_256gb) return true;
    if (partition_id == kSavio4_htc_512gb) return true;

    return false;
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

static int PrintShebang(FILE *file) { return fprintf(file, "#!/bin/sh\n"); }

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

static int PrintExportOmpNumThreads(FILE *file, int num_threads) {
    return fprintf(file, "export OMP_NUM_THREADS=%d\n", num_threads);
}

static int PrintSolveCommand(FILE *file, int num_processes,
                             int cpus_per_proccess, ReadOnlyString game_name,
                             int variant_id) {
    return fprintf(
        file, "srun -n %d -c %d --cpu_bind=cores bin/gamesman solve %s %d\n",
        num_processes, cpus_per_proccess, game_name, variant_id);
}
