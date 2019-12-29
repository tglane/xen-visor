#ifndef RM_RESSOURCE_MODEL_H
#define RM_RESSOURCE_MODEL_H

#include <libxl.h>

/**
 * RM_RESSOURCE_MODEL provides inforamtion about the current status of the load of the domains currently running
 */

/**
 * 
 */
struct domain_load {
    int dom_id;
    double cpu_load;
    double mem_load;
};
typedef struct domain_load domain_load_t;

int RM_RESSOURCE_MODEL_init(void);

void RM_RESSOURCE_MODEL_free(void);

double RM_RESSOURCE_MODEL_get_domain_cpuload(int dom_id);

double RM_RESSOURCE_MODEL_get_domain_memload(int dom_id);

int RM_RESSOURCE_MODEL_get_used_cpus(void);

int64_t RM_RESSOURCE_MODEL_get_used_memory(void);

int RM_RESSOURCE_MODEL_update(libxl_dominfo* dom_list, int num_domains);

domain_load_t* RM_RESSOURCE_MODEL_get_ressource_data(int* num_entries);

#endif

