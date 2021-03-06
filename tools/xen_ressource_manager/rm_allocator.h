#ifndef RM_ALLOCATOR_H
#define RM_ALLOCATOR_H

#include <rm_ressource_model.h>
#include <libxl.h>

/**
 * RM_ALLOCATOR is used to do the actual ressource adjustment of the domains
 * Domains request more ressources or request the reduction of their ressources depending on the load
 * Then it tries to fulfill the requests of the domains where domains with higher load are priorized
 */

/**
 * Sets an ask to adjust the ressources of a domain if necessary
 *
 * Returns 0 if successful without errors and -1 else
 * Parameter dom_load pointer do the load data structure of one domain
 * Parameter dom_info pointer to the libxl_dominfo structure of one domain 
 */
int RM_ALLOCATOR_allocation_ask(domain_load_t* dom_load, libxl_dominfo dom_info);

/**
 * Adjusts the ressource distribution by asks made by the domains previously in RM_ALLOCATOR_allocation_ask(...)
 *
 * Returns 0 if any adjustment was made successful and -1 if any error occurs during adjustment or no adjustment was made
 * Parameter dom_list array of libxl_dominfo structures of length num_domains
 * Parameter dom_load array of domain_load_t structures of length num_domains
 * Parameter num_domains number of domains in the above mentioned arrays
 */
int RM_ALLOCATOR_ressource_adjustment(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains);

#endif

