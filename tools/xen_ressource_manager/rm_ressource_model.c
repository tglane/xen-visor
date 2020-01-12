#include <rm_ressource_model.h>

#include <stdlib.h>
#include <rm_xenstore.h>
#include <rm_xl.h>
#include <rm_allocator.h>

#define WEIGHT 0.75

//static domain_load_t* ressource_data;
//static int max_domain_id = 0;

static unsigned int host_cpus_used = 0;
static uint64_t host_memory_used = 0;

domain_load_t* RM_RESSOURCE_MODEL_init(int* max_domain_id)
{
    /*if(ressource_data == NULL)
    {
        ressource_data = malloc(sizeof(domain_load_t));
        if(ressource_data == NULL)
            return -1;
        ressource_data[0] = (domain_load_t) {-1, -1, 0, 0, NULL, 0, -1.0, -1.0};
    }
    return 0;*/

    domain_load_t* ressource_data = malloc(sizeof(domain_load_t));
    if(ressource_data == NULL)
        return NULL;

    ressource_data[0] = (domain_load_t) {-1, -1, 0, 0, NULL, 0, -1.0, -1.0};
    *max_domain_id = 0;
    return ressource_data;
}

void RM_RESSOURCE_MODEL_free(domain_load_t* ressource_data, int max_domain_id)
{
    if(ressource_data != NULL)
    {
        int i;
        for(i = 0; i < max_domain_id; i++)
        {
            if(ressource_data[i].vcpu_info != NULL)
                libxl_vcpuinfo_list_free(ressource_data[i].vcpu_info, ressource_data[i].num_vcpus);
        }

        free(ressource_data);
    }
}

double RM_RESSOURCE_MODEL_get_domain_cpuload(domain_load_t* ressource_data, int max_domain_id, int dom_id)
{
    if(ressource_data == NULL)
        return -1;

    if(dom_id < 0 || dom_id > max_domain_id || ressource_data[dom_id].dom_id == -1)
        return -1;

    return ressource_data[dom_id].cpu_load;
}

double RM_RESSOURCE_MODEL_get_domain_memload(domain_load_t* ressource_data, int max_domain_id, int dom_id)
{
    if(ressource_data == NULL)
        return -1;

    if(dom_id < 0 || dom_id > max_domain_id || ressource_data[dom_id].dom_id == -1)
        return -1;

    return ressource_data[dom_id].mem_load;
}

int RM_RESSOURCE_MODEL_get_domain_priority(domain_load_t* ressource_data, int max_domain_id, int dom_id)
{
    if(ressource_data == NULL)
        return -1;

    if(dom_id < 0 || dom_id > max_domain_id || ressource_data[dom_id].dom_id == -1)
        return -1;

    return ressource_data[dom_id].priority;
}

int RM_RESSOURCE_MODEL_get_domain_vcpucount(domain_load_t* ressource_data, int max_domain_id, int dom_id)
{
    if(ressource_data == NULL)
        return -1;

    if(dom_id < 0 || dom_id > max_domain_id || ressource_data[dom_id].dom_id == -1)
        return -1;

    return ressource_data[dom_id].vcpu_used;
}

int RM_RESSOURCE_MODEL_get_used_cpus(void)
{
    return host_cpus_used;
}

int64_t RM_RESSOURCE_MODEL_get_used_memory(void)
{
    return host_memory_used;
}

int RM_RESSOURCE_MODEL_update(domain_load_t* ressource_data, int max_domain_id, libxl_dominfo* dom_list, int num_domains)
{
    int i, j, priority, vcpu_used;
    int64_t mem_used;
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
            
            for(j = max_domain_id + 1; j <= dom_list[i].domid; j++)
            {
                ressource_data[j] = (domain_load_t) {-1, -1, 0, 0, NULL, 0, -1.0, -1.0};
            }
            max_domain_id = dom_list[i].domid;
        }

        memload = RM_XENSTORE_read_domain_memload(dom_list[i].domid);
        cpuload = RM_XENSTORE_read_domain_cpuload(dom_list[i].domid); 
        priority = RM_XENSTORE_read_domain_priority(dom_list[i].domid);

        vcpu_used = dom_list[i].vcpu_online;
        mem_used = dom_list[i].current_memkb + dom_list[i].outstanding_memkb;

        host_cpus_used += vcpu_used;
        host_memory_used += mem_used; 

        // Save current domain load and calculate average load only if there is load data
        if(memload < 0 || cpuload < 0)
        {

            if(ressource_data[dom_list[i].domid].vcpu_info != NULL)
                libxl_vcpuinfo_list_free(ressource_data[dom_list[i].domid].vcpu_info, ressource_data[dom_list[i].domid].num_vcpus);

            ressource_data[dom_list[i].domid].dom_id = -1;
            ressource_data[dom_list[i].domid].priority = -1;
            ressource_data[dom_list[i].domid].vcpu_used = 0;
            ressource_data[dom_list[i].domid].num_vcpus = 0;
            ressource_data[dom_list[i].domid].vcpu_info = NULL;
            ressource_data[dom_list[i].domid].mem_used = 0;
            ressource_data[dom_list[i].domid].cpu_load = 0.0;
            ressource_data[dom_list[i].domid].mem_load = 0.0;
        }
        else
        {
            int num_vcpus;
            // freed in RM_RESSOURCE_MODEL_free function
            libxl_vcpuinfo* vcpu_info = RM_XL_get_vcpu_list(dom_list[i].domid, &num_vcpus);

            ressource_data[dom_list[i].domid].dom_id = dom_list[i].domid;
            ressource_data[dom_list[i].domid].priority = priority;
            ressource_data[dom_list[i].domid].vcpu_used = vcpu_used;
            ressource_data[dom_list[i].domid].num_vcpus = num_vcpus;
            ressource_data[dom_list[i].domid].vcpu_info = vcpu_info;
            ressource_data[dom_list[i].domid].mem_used = mem_used;

            // Calculate exponential moving average
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

/*domain_load_t* RM_RESSOURCE_MODEL_get_ressource_data(int* num_entries)
{
    *num_entries = max_domain_id + 1;
    return ressource_data;
}*/

