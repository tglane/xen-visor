#ifndef RM_NUMA_MANAGER_H
#define RM_NUMA_MANAGER_H

#include <rm_xl.h>

int RM_NUMA_MANAGER_init_numa_info(void);

void RM_NUMA_MANAGER_close(void);

int RM_NUMA_MANAGER_update_vcpu_placing(libxl_dominfo* dom_list, libxl_dominfo* s_dom_list, int num_domains);

#endif

