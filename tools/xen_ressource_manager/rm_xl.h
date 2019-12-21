#ifndef RM_XL_H
#define RM_XL_H

#include <stdlib.h>
#include <stdint.h>
#include <libxl.h>

#define MEM_STEP 100000
#define DOMAIN_MIN_MEM 300000

int RM_XL_init(void);

void RM_XL_close(void);

/*
 * Do not forget to free returned int array!
 */
libxl_dominfo* RM_XL_get_domain_list(int* num_dom_out);

int RM_XL_get_host_cpu(void);

int64_t RM_XL_get_host_mem_total(void);

libxl_numainfo* RM_XL_get_numa_topology(int* num_out);

libxl_cputopology* RM_XL_get_cpu_topology(int* num_out);

int RM_XL_change_vcpu(int domid, int change_vcpus);

int RM_XL_change_memory(int domid, int64_t change_kb);

#endif

