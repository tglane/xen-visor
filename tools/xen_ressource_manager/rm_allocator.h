#ifndef RM_ALLOCATOR_H
#define RM_ALLOCATOR_H

#include <rm_ressource_model.h>
#include <libxl.h>

int RM_ALLOCATOR_allocation_ask(domain_load_t* dom_load, libxl_dominfo dom_info);

int RM_ALLOCATOR_ressource_adjustment(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains);

#endif

