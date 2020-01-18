#ifndef RM_RESSOURCE_MODEL_H
#define RM_RESSOURCE_MODEL_H

#include <libxl.h>

/**
 * RM_RESSOURCE_MODEL provides inforamtion about the current status of the load of the domains currently running
 */

/**
 * Struct containing load information for one domain
 */
struct domain_load {
    int dom_id;
    int priority;
    int vcpu_used;
    int num_vcpus;
    libxl_vcpuinfo* vcpu_info;
    int64_t mem_used;
    double cpu_load;
    double mem_load;
};
typedef struct domain_load domain_load_t;

/**
 * Initializes the structure that stores ressource load data for the running domains
 * This function must run before using other functions from this module 
 *
 * Returns 0 if initialization was successful and -1 if not
 */
int RM_RESSOURCE_MODEL_init(void);

/**
 * Frees the struct containing the ressource data for all domains
 */
void RM_RESSOURCE_MODEL_free(void);

/**
 * Returns the current cpu load of a domain
 * Returns -1 if the given id belongs to no active domain
 *
 * Returns load in percent (0.00 - 100.00) or -1
 * Parameted dom_id id of domain
 */
double RM_RESSOURCE_MODEL_get_domain_cpuload(int dom_id);

/**
 * Returns the current memory load of a domain
 * Returns -1 if the given id belongs to no active domain
 *
 * Returns load in percent (0.00 - 100.00) or -1
 * Parameted dom_id id of domain
 */
double RM_RESSOURCE_MODEL_get_domain_memload(int dom_id);

/**
 * Returns the priority of a domain
 * Returns -1 if the given id belongs to no active domain
 *
 * Returns priority from 1 (low) to 5 (high)
 * Parameter dom_id id of domain
 */
int RM_RESSOURCE_MODEL_get_domain_priority(int dom_id);

/**
 * Returns the number of currently online vCPUs of a domain
 * Returns -1 if the given id belongs to no active domain
 * Parameter dom_id is the id of the domain
 */
int RM_RESSOURCE_MODEL_get_domain_vcpucount(int dom_id);

/**
 * Returns the number of kb memory currently addressed to a given domain
 * Returns -1 if the given id belongs to no active domain
 * Parameter dom_id is the id of the domains
 */
int64_t RM_RESSOURCE_MODEL_get_domain_memuseage(int dom_id);

/**
 * Returns the currently used physical cpus by domains
 */
int RM_RESSOURCE_MODEL_get_used_cpus(void);

/**
 * Returns amount of memory that is currently used by domains in kilobyte
 */
int64_t RM_RESSOURCE_MODEL_get_used_memory(void);

/**
 * Updates the static structure with the new ressource data from the XenStore
 * The load is updated by using the exponential moving average
 *
 * Returns 0 if update was successful and -1 if not
 * Parameter dom_list array with all a libxl_dominfo structure per running domain
 * Parameter num_domains length of dom_list
 */
int RM_RESSOURCE_MODEL_update(libxl_dominfo* dom_list, int num_domains);

/**
 * Returns a pointer to the static array containing the ressource load data of the domains
 * Parameter num_entries pointer in which the number of entries in the ressource load array is written
 */
domain_load_t* RM_RESSOURCE_MODEL_get_ressource_data(int* num_entries);

#endif

