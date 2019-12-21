#include <rm_numa_manager.h>

int RM_NUMA_MANAGER_update_vcpu_placing(libxl_dominfo* dom_list, int num_domains)
{
    int i, num_cpu;
    libxl_cputopology* cpu_top = RM_XL_get_cpu_topology(&num_cpu);

    // TODO get current cpu topology and put domains vCPUs clever on pCPUs
    for(i = 0; i < num_domains; i++)
    {
        
    }

    libxl_cputopology_list_free(cpu_top, num_cpu);
    return 0;
}

