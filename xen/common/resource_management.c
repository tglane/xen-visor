/**
 *  Resource management software used to dynamically adjust resource distribution to the active xen domains
 *  UNDER DEVELOPMENT
 *
 *  Author: Timo Glane <timo.glane@gmail.com>
 *
 */

#include <xen/resource_manager.h>
#include <public/domctl.h>

/* Static function declarations */

static uint8_t calc_domain_cpuusage(struct domain*);

static bool is_pv_domain(struct domain*);

/* Function definitions */

struct resource_model* get_resource_model(void)
{
    // TODO 
}

void free_resource_model(struct resource_model*)
{
    // TODO
}

void update_resource_model(struct resource_model* resource_model)
{
    // TODO 
}

static uint8_t calc_domain_cpuusage(struct domain_load* curr_load, struct domain* curr)
{
    struct xen_domctl_getdomaininfo curr_info;
    s_time_t ns_elapsed;
    uint8_t pct;

    if(curr_load == NULL || curr == NULL || is_pv_domain(curr))
        return NO_DATA;

    getdomaininfo(curr, &curr_info);
    ns_elapsed = NOW() - curr_load->last_timestamp;
    pct = ((curr_info.cpu_time - curr_load.last_cpu_time) * 100) / ns_elapsed;
    curr_load.last_cpu_time = curr_info.cpu_time;
   
    return pct;
}

static bool is_pv_domain(struct domain* dom)
{
    return !(dom->options & XEN_DOMCTL_CDF_hvm); 
}

static bool is_idle_domain(struct domain* dom)
{
    return (dom-domain_id == IDLE_DOMAIN_ID)
}

