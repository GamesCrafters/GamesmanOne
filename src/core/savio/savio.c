#include "core/savio/savio.h"

const SavioPartition kSavioPartitions[kNumSavioPartitions] = {
    {
        .name = "savio3",
        .desc = "savio3 32-CPU",
        .num_nodes = 112,
        .num_cpu = 32,
        .mem_gb = 96,
        .su_per_core_hour = 1.0,
        .per_node_allocation = true,
    },
    {
        .name = "savio3",
        .desc = "savio3 40-CPU",
        .num_nodes = 80,
        .num_cpu = 40,
        .mem_gb = 96,
        .su_per_core_hour = 1.0,
        .per_node_allocation = true,
    },
    {
        .name = "savio3_htc",
        .desc = "savio3_htc",
        .num_nodes = 24,
        .num_cpu = 40,
        .mem_gb = 384,
        .su_per_core_hour = 2.67,
        .per_node_allocation = false,
    },
    {
        .name = "savio4_htc",
        .desc = "savio4_htc 256GB mem",
        .num_nodes = 84,
        .num_cpu = 56,
        .mem_gb = 256,
        .su_per_core_hour = 3.67,
        .per_node_allocation = false,
    },
    {
        .name = "savio4_htc",
        .desc = "savio4_htc 512GB mem",
        .num_nodes = 24,
        .num_cpu = 56,
        .mem_gb = 512,
        .su_per_core_hour = 3.67,
        .per_node_allocation = false,
    },
};

ConstantReadOnlyString kSavioDefaultAccount = "fc_gamecrafters";
const int kSavioDefaultPartition = kSavio4_htc_256gb;
const int kSavioNumNodesMax = 24;
const int kSavioDefaultNumTasksPerNode = 1;
ConstantReadOnlyString kSavioDefaultTimeLimit = "72:00:00";

int SavioGetNumCpuPerTask(int num_cpu, int num_tasks_per_node) {
    if (num_tasks_per_node == 0) return 0;
    return num_cpu / num_tasks_per_node;
}

int SavioGetNumTasksPerNode(int num_cpu, int cpus_per_task) {
    if (cpus_per_task == 0) return 0;
    return num_cpu / cpus_per_task;
}
