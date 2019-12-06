#ifndef RM_ALLOCATOR_H
#define RM_ALLOCATOR_H

#include <rm_ressource_model.h>
#include <libxl.h>

int RM_ALLOCATOR_allocation_ask(domain_load_t* dom_load);

int RM_ALLOCATOR_ressource_adjustment(libxl_dominfo* dom_list, int num_domains);

#endif

