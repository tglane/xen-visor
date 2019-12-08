#include <rm_ressource_model.h>

#include <stdlib.h>
#include <rm_xenstore.h>

#include <stdio.h>

#define WEIGHT 0.75

static domain_load_t* ressource_data;
static int max_domain_id = 0;

static unsigned int host_cpus_used = 0;
static uint64_t host_memory_used = 0;

void RM_RESSOURCE_MODEL_init(void)
{
    if(ressource_data == NULL)
    {
        ressource_data = malloc(sizeof(domain_load_t));
        ressource_data[0] = (domain_load_t) {-1, 0.0, 0.0};
    }
}

void RM_RESSOURCE_MODEL_free(void)
{
    if(ressource_data != NULL)
        free(ressource_data);
}

double RM_RESSOURCE_MODEL_get_domain_cpuload(int dom_id)
{
    if(ressource_data == NULL)
        return -1;

    if(dom_id < 0)
        return -1;

    return ressource_data[dom_id].cpu_load;
}

double RM_RESSOURCE_MODEL_get_domain_memload(int dom_id)
{
    if(ressource_data == NULL)
        return -1;

    if(dom_id < 0)
        return -1;

    return ressource_data[dom_id].mem_load;
}


int RM_RESSOURCE_MODEL_get_used_cpus(void)
{
    return host_cpus_used;
}

int64_t RM_RESSOURCE_MODEL_get_used_memory(void)
{
    return host_memory_used;
}

int RM_RESSOURCE_MODEL_update(libxl_dominfo* dom_list, int num_domains)
{
    int i, j;
    double memload, cpuload;

    if(!RM_XENSTORE_initialized())
    {
        return -1;
    }

    host_cpus_used = 0;
    host_memory_used = 0;

    for(i = 0; i < num_domains; i++)
    {
        // Realloc memory for ressource_data if max_domain_id is lower than current id
        if(dom_list[i].domid > max_domain_id)
        {
            ressource_data = realloc(ressource_data, (dom_list[i].domid + 1) * sizeof(domain_load_t));
            if(ressource_data == NULL)
                return -1;

            for(j = max_domain_id + 1; j < dom_list[i].domid; j++)
            {
                ressource_data[j] = (domain_load_t) {-1, 0.0, 0.0};
            }

            max_domain_id = dom_list[i].domid;
        }

        memload = RM_XENSTORE_read_domain_memload(dom_list[i].domid);
        cpuload = RM_XENSTORE_read_domain_cpuload(dom_list[i].domid); 

        host_cpus_used += dom_list[i].vcpu_online;
        host_memory_used += (dom_list[i].current_memkb + dom_list[i].outstanding_memkb);

        // Save current domain load and calculate average load only if there is load data
        if(memload < 0 || cpuload < 0)
        {
            ressource_data[dom_list[i].domid].dom_id = -1;
            ressource_data[dom_list[i].domid].cpu_load = 0;
            ressource_data[dom_list[i].domid].mem_load = 0;
        }
        else
        {
            // Calculate exponential moving average
            ressource_data[dom_list[i].domid].dom_id = dom_list[i].domid;
            ressource_data[dom_list[i].domid].cpu_load = 
                (WEIGHT * cpuload) + (1.0 - WEIGHT) * ressource_data[dom_list[i].domid].cpu_load;
            ressource_data[dom_list[i].domid].mem_load = 
                (WEIGHT * memload) + (1.0 - WEIGHT) * ressource_data[dom_list[i].domid].mem_load;

            // Moving average
            /*ressource_data[dom_list[i].domid].iterations++;

            ressource_data[dom_list[i].domid].dom_id = dom_list[i].domid;
            
            ressource_data[dom_list[i].domid].cpu_load = ressource_data[dom_list[i].domid].cpu_load + 
                ((cpuload - ressource_data[dom_list[i].domid].cpu_load) / ressource_data[dom_list[i].domid].iterations);
            
            ressource_data[dom_list[i].domid].mem_load = ressource_data[dom_list[i].domid].mem_load + 
                ((memload - ressource_data[dom_list[i].domid].mem_load) / ressource_data[dom_list[i].domid].iterations);*/
        }
    }

    return 0;
}

domain_load_t* RM_RESSOURCE_MODEL_get_ressource_data(int* num_entries)
{
    *num_entries = max_domain_id + 1;
    return ressource_data;
}

