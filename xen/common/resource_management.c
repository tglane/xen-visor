/**
 *  Resource management software used to dynamically adjust resource distribution to the active xen domains
 *  UNDER DEVELOPMENT
 *
 *  Author: Timo Glane <timo.glane@gmail.com>
 *
 */

#include <xen/resource_management.h>
#include <public/domctl.h>


/* Static function declarations */

static int32_t update_domain_cpuusage(struct domain_load*, struct domain*);

static int32_t update_domain_memusage(struct domain_load*, struct domain*);


/* Function definitions */

struct resource_model* create_resource_model(void)
{
    struct resource_model* rm = xmalloc(struct resource_model);
    rm->size = 0;
    rm->load_data = NULL;
    return rm;
}

void free_resource_model(struct resource_model* res_model)
{
    if(res_model == NULL)
        return;

    xfree(res_model->load_data);
    xfree(res_model);

    res_model = NULL;
}

void update_resource_model(struct resource_model* res_model)
{
    struct domain* dom_list_ptr;

    // TODO 
    // - get domain list as struct domain* for all existing domains
    // - update memory load per domain
    // - !!! Destroy entry in the resource_model when a domain is destroyed in domain.c

    if(res_model == NULL || res_model->size == 0)
        return;

    /* struct domain* domain_list used from shed.h */
    for(dom_list_ptr = domain_list; dom_list_ptr != NULL; dom_list_ptr = dom_list_ptr->next_in_list)
    {
        uint8_t cpu_pct, mem_pct;

        cpu_pct = update_domain_cpuusage(&res_model->load_data[dom_list_ptr->domain_id], dom_list_ptr);
        if(cpu_pct >= CPU_ADD_THRESHOLD)
        {}
        else if(cpu_pct <= CPU_REMOVE_THRESHOLD)
        {}

        mem_pct = update_domain_memusage(&res_model->load_data[dom_list_ptr->domain_id], dom_list_ptr);
        if(mem_pct >= MEM_ADD_THRESHOLD)
        {}
        else if(mem_pct <= MEM_REMOVE_THRESHOLD)
        {}
    }
}

static int32_t update_domain_cpuusage(struct domain_load* curr_load, struct domain* curr)
{
    struct xen_domctl_getdomaininfo curr_info;
    s_time_t ns_elapsed, now;
    uint8_t pct;

    if(curr_load == NULL || curr == NULL || !is_pv_domain(curr))
        return NO_DATA;

    getdomaininfo(curr, &curr_info);
    now = NOW();
    ns_elapsed = now - curr_load->last_timestamp;
    pct = ((curr_info.cpu_time - curr_load->last_cpu_time) * 100) / ns_elapsed;
    curr_load->last_cpu_time = curr_info.cpu_time;
    curr_load->last_timestamp = now;
    curr_load->cpu_load = pct;
   
    return pct;
}

static int32_t update_domain_memusage(struct domain_load* curr_load, struct domain* curr)
{
    // TODO
    return 0;
}
