#ifndef GAMESMANONE_CORE_SAVIO_SAVIO_H_
#define GAMESMANONE_CORE_SAVIO_SAVIO_H_

#include "core/types/gamesman_types.h"

enum SavioSettingsConstants {
    kSavioPartitionNameLengthMax = 31,
    kSavioPartitionDescLengthMax = 31,
    kSavioJobNameLengthMax = 31,
    kSavioAccountNameLengthMax = 31,

    // Time limit must be of the format "hh:mm:ss".
    kSavioTimeLimitLengthMax = 8,
};

typedef struct SavioPartition {
    ConstantReadOnlyString name;
    ConstantReadOnlyString desc;
    double su_per_core_hour;
    int num_nodes;
    int num_cpu;
    int mem_gb;
    bool per_node_allocation;
} SavioPartition;

enum SavioPartitions {
    kSavio3_16c32t,
    kSavio3_20c40t,
    kSavio3_htc,
    kSavio4_htc_256gb,
    kSavio4_htc_512gb,
    kNumSavioPartitions,
};

typedef struct SavioJobSettings {
    char game_name[kGameNameLengthMax + 1];
    int game_variant_id;
    char job_name[kSavioJobNameLengthMax + 1];
    char account[kSavioAccountNameLengthMax + 1];
    int partition_id;
    int num_nodes;
    int ntasks_per_node;
    char time_limit[kSavioTimeLimitLengthMax + 1];
    bool bind_omp_threads_to_cores;
} SavioJobSettings;

extern const SavioPartition kSavioPartitions[kNumSavioPartitions];
extern ConstantReadOnlyString kSavioDefaultAccount;
extern const int kSavioNumNodesMax;
extern const int kSavioDefaultNumTasksPerNode;
extern ConstantReadOnlyString kSavioDefaultTimeLimit;

int SavioGetNumCpuPerTask(int num_cpu, int num_tasks_per_node);
int SavioGetNumTasksPerNode(int num_cpu, int cpus_per_task);

#endif  // GAMESMANONE_CORE_SAVIO_SAVIO_H_
