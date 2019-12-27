#ifndef RM_XL_H
#define RM_XL_H

#include <stdlib.h>
#include <stdint.h>
#include <libxl.h>

/* Define memory values for domains */
#define MEM_STEP 100000
#define DOMAIN_MIN_MEM 300000

/* Define vCPU pin types */
#define VCPU_PIN_HARD 1
#define VCPU_PIN_SOFT 0
#define VCPU_UNPIN 3

int RM_XL_init(void);

void RM_XL_close(void);

/*
 * Do not forget to free returned int array!
 */
libxl_dominfo* RM_XL_get_domain_list(int* num_dom_out);

libxl_vcpuinfo* RM_XL_get_vcpu_list(int domid, int* num_vcpu_out);

int RM_XL_get_host_cpu(void);

int64_t RM_XL_get_host_mem_total(void);

libxl_physinfo* RM_XL_get_physinfo(void);

libxl_numainfo* RM_XL_get_numa_topology(int* num_out);

libxl_cputopology* RM_XL_get_cpu_topology(int* num_out);

int RM_XL_pin_vcpu(int dom_id, int vcpu_id, int pcpu_id, int pin_type);

int RM_XL_change_vcpu(int domid, int change_vcpus);

int RM_XL_change_memory(int domid, int64_t change_kb);

#endif

