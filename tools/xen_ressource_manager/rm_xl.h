#ifndef RM_XL_H
#define RM_XL_H

#include <stdlib.h>
#include <stdint.h>
#include <libxl.h>

/**
 * RM_XL is used to access functions from libxl in a save, easy and constant way
 * Contains functions to get status information about the host machine or domains
 * Changing the hardware resources of domains is also done here
 */

/* Define memory values for domains */
#define MEM_STEP 100000
#define DOMAIN_MIN_MEM 300000

/* Define vCPU pin types */
#define VCPU_PIN_HARD 1
#define VCPU_PIN_SOFT 0
#define VCPU_UNPIN 3

/**
 * Initializes the context to use functions from libxl
 * Accessing libxl won't work without previous initialization
 *
 * Returns 0 if initialization was ok and -1 if not
 */
int RM_XL_init(void);

/**
 * Free all structures that were created with RM_XL_init()
 */
void RM_XL_close(void);

/**
 * Get a list of all currently active domains with information about their status
 * Returned pointer needs to be freed with libxl_dominfo_list_free(libxl_dominfo*, size)
 * Do not forget to free returned array!
 *
 * Returns array of libxl_dominfo structures with length num_dom_out
 * Parameter num_dom_out contains the number of domains returned by this function
 */
libxl_dominfo* RM_XL_get_domain_list(int* num_dom_out);

/**
 * Get a list of all vCPUs and status information of a domain 
 * Returned pointer needs to be freed with libxl_vcpuinfo_list_free(libxl_vcpuinfo*, size)
 * Do not forget to free returned array!
 *
 * Returns array of libxl_vcpuinfo structures with length num_vcpu_out
 * Parameter domid is the id of the domain to get vcpu info from
 * Parameter num_vcpu_out contains the number of vcpus returned by this function
 */
libxl_vcpuinfo* RM_XL_get_vcpu_list(int domid, int* num_vcpu_out);

/**
 * Get the current number of cpus running in the host machine that can be given to domains
 *
 * Returns number of currently running cpus in the host machine
 */
int RM_XL_get_host_cpu(void);

/**
 * Get the nummber of total available memory in the host machine in kilobyte
 *
 * Returns number of total available memory
 */
int64_t RM_XL_get_host_mem_total(void);

/**
 * Get all physical information about the host machine
 * Do not forget to free the returned pointer
 *
 * Returns pointer to a structure libxl_physinfo 
 */
libxl_physinfo* RM_XL_get_physinfo(void);

/**
 * Get a list of numanodes and information about the nodes
 * Returned pointer needs to be freed with libxl_numainfo_list_free(libxl_numainfo*, size)
 * Do not forget to free returned array!
 *
 * Returns array of libxl_numainfo structures with length num_out
 * Parameter num_out contains the number of numa nodes returned by this function
 */
libxl_numainfo* RM_XL_get_numa_topology(int* num_out);

/**
 * Get a list of physical cpus and the topology of them
 * Returned pointer needs to be freed with libxl_cputopology_list_free(libxl_cputopology*, size)
 * Do not forget to free returned array!
 *
 * Returns array of libxl_cputopology structures with length num_out
 * Parameter num_out contains the number of cpus returned by this function
 */
libxl_cputopology* RM_XL_get_cpu_topology(int* num_out);

/*
 * Pin or unpin a vCPU of a domain to/from a physical cpu of the host machine
 *
 * Returns 0 if pinning/unpinning was successful
 * Parameter dom_id id of domain
 * Parameter vcpu_id id of the vcpu of the specified domain you want to pin/unpin
 * Parameter pcpu_id id of the physical cpu to pin on; -1 if you want to unpin a vcpu
 * Parameter pin_type {VCPU_PIN_SOFT | VCPU_PIN_HARD | VCPU_UNPIN}
 */
int RM_XL_pin_vcpu(int dom_id, int vcpu_id, int pcpu_id, int pin_type);

/**
 * Change the number of vCPUs of a domain by an offset
 *
 * Returns 0 if operation was successful and -1 if not (e.g. not enough physical CPUs available)
 * Parameter domid id of domain
 * Parameter change_vcpus number of vCPUs to be added or removed (1 -> add one vCPU; -1 -> remove one vCPU)
 */
int RM_XL_change_vcpu(int domid, int change_vcpus);

/**
 * Change amount of memory a domain can use by an offset in kilobyte
 * 
 * Returns 0 if successful and -1 if not (e.g. not enough free physical memory)
 * Parameter domid id of domain
 * Parameter change_kb amount of memory to change in kilobyte (1000 -> add 1000kB; -1000 -> remove 1000kB of memory)
 */
int RM_XL_change_memory(int domid, int64_t change_kb);

#endif

