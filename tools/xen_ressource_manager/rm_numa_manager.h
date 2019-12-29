#ifndef RM_NUMA_MANAGER_H
#define RM_NUMA_MANAGER_H

#include <rm_xl.h>

/**
 * RM_NUMA_MANAGER is used to pin the vCPUs of the domains on physical CPUs paying attention to the NUMA 
 * topology of the system
 * Tries to put all vCPUs of one domain on the same node
 *
 * Currently using a greey approach where the domains with the most vCPUs are pinned first on the node with the 
 * most free physical CPUs
 */

/**
 * Initializes the structure node_info containing the numa node infos with the cpu topology
 *
 * Returns 0 if initialization was successful and -1 if not
 */
int RM_NUMA_MANAGER_init_numa_info(void);

/**
 * Frees all memory allocated by the static structure node_info
 */
void RM_NUMA_MANAGER_close(void);

/**
 * Function that does the actual NUMA aware placement of domains vCPUs using a greedy approach as described above
 *
 * Returns 0 if placement was successful and -1 if not
 * Parameter dom_list array of libxl_dominfo structure of length num_domains
 * Parameter s_dom_list same array as dom_list but sorted by number of vCPUs the domains use
 * Parameter num_domains number of domains in the arrays mentioned before
 */
int RM_NUMA_MANAGER_update_vcpu_placing(libxl_dominfo* dom_list, libxl_dominfo* s_dom_list, int num_domains);

#endif

