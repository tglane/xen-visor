#include <rm_allocator.h>

#include <stdio.h>
#include <syslog.h>
#include <rm_xl.h>

#define MIN_DOMAIN_MEM 300000
#define MEM_STEP 100000

static int RM_ALLOCATOR_resolve_cpu_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains);

static int RM_ALLOCATOR_resolve_mem_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains);

struct allocation_ask {
    int cpu_ask;
    int mem_ask;
};
typedef struct allocation_ask allocation_ask_t;

struct allocation_summary {
    unsigned int cpu_add;
    unsigned int cpu_reduce;
    unsigned int mem_add;
    unsigned int mem_reduce;
};
typedef struct allocation_summary allocation_summary_t;

static allocation_ask_t* alloc_ask;
static unsigned int num_domains = 0;
static allocation_summary_t alloc_summary = (allocation_summary_t) {0, 0, 0, 0};

int RM_ALLOCATOR_allocation_ask(domain_load_t* domain, libxl_dominfo dom_info)
{
    int64_t domain_memory;

    syslog(LOG_NOTICE, "ID: %d; CPU: %f; MEM: %f\n", domain->dom_id, domain->cpu_load, domain->mem_load);
   
    if(domain->dom_id >= num_domains)
    {
        alloc_ask = realloc(alloc_ask, (domain->dom_id + 1) * sizeof(allocation_ask_t));
        if(alloc_ask == NULL)
            return -1;
    }
    
    // Ressource allocation ask for vcpus
    if(domain->cpu_load > 70 && (dom_info.vcpu_online + 1 <= RM_XL_get_host_cpu()))
    { 
        alloc_ask[domain->dom_id].cpu_ask = 1;
        alloc_summary.cpu_add++;
    }
    else if(domain->cpu_load < 20 && (dom_info.vcpu_online - 1 > 0))
    {
        alloc_ask[domain->dom_id].cpu_ask = -1;
        alloc_summary.cpu_reduce++;
    }
    else 
    {
        alloc_ask[domain->dom_id].cpu_ask = 0;
    }

    // Ressource allocation ask for memory
    domain_memory = dom_info.current_memkb + dom_info.outstanding_memkb;
    if(domain->mem_load > 90 && ((domain_memory + MEM_STEP) <= RM_XL_get_host_mem_total()))
    {
        alloc_ask[domain->dom_id].mem_ask = 1;
        alloc_summary.mem_add++; 
    }
    else if(domain->mem_load < 40 && ((domain_memory - MEM_STEP) > MIN_DOMAIN_MEM))
    {
        alloc_ask[domain->dom_id].mem_ask = -1;
        alloc_summary.mem_reduce++;
    }
    else
    {
        alloc_ask[domain->dom_id].mem_ask = 0;
    }
    
    syslog(LOG_NOTICE, "ID: %d; CPU_ASK: %d; MEM_ASK: %d\n", domain->dom_id, alloc_ask[domain->dom_id].cpu_ask, alloc_ask[domain->dom_id].mem_ask);
    
    return 0;
}

int RM_ALLOCATOR_ressource_adjustment(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains)
{
    int i;

    syslog(LOG_NOTICE, "### Ressource adjustment ###\n");
    syslog(LOG_NOTICE, "Host CPUs: %d\n", RM_XL_get_host_cpu());
    syslog(LOG_NOTICE, "Used CPUs: %d\n", RM_RESSOURCE_MODEL_get_used_cpus());
    syslog(LOG_NOTICE, "alloc_summary.cpu_add: %d\n", alloc_summary.cpu_add);
    syslog(LOG_NOTICE, "alloc_summary.cpu_reduce: %d\n", alloc_summary.cpu_reduce);

    // Resolve CPU allocations
    if((alloc_summary.cpu_reduce >= alloc_summary.cpu_add) || 
        (alloc_summary.cpu_add - alloc_summary.cpu_reduce <= RM_XL_get_host_cpu() - RM_RESSOURCE_MODEL_get_used_cpus()))
    {
        // Resolve all allocation asks
        for(i = 0; i < num_domains; i++)
        {
            if(alloc_ask[dom_list[i].domid].cpu_ask != 0)
            {
                syslog(LOG_NOTICE, "direct CPU_SET for id: %d\n", dom_list[i].domid);
                RM_XL_change_vcpu(dom_list[i].domid, alloc_ask[dom_list[i].domid].cpu_ask);
            }
        }
    }
    else
    {
        RM_ALLOCATOR_resolve_cpu_allocations(dom_list, dom_load, num_domains);
    }

    // Resolve MEM allocation
    if((alloc_summary.mem_reduce >= alloc_summary.mem_add) || 
        (alloc_summary.mem_add - alloc_summary.mem_reduce <= RM_XL_get_host_mem_total() - RM_RESSOURCE_MODEL_get_used_memory()))
    {
        for(i = 0; i < num_domains; i++)
        {
            if(alloc_ask[dom_list[i].domid].mem_ask != 0)
            { 
                syslog(LOG_NOTICE, "direct MEM_SET for id: %d with %d\n", dom_list[i].domid, MEM_STEP * alloc_ask[dom_list[i].domid].mem_ask);
                RM_XL_change_memory(dom_list[i].domid, MEM_STEP * alloc_ask[dom_list[i].domid].mem_ask);
            }
        }
    }
    else
    {
        RM_ALLOCATOR_resolve_mem_allocations(dom_list, dom_load, num_domains);
    }

    alloc_summary = (allocation_summary_t) {0, 0, 0, 0};
    return 0;
}

// TODO add domain priorities
static int RM_ALLOCATOR_resolve_cpu_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains)
{
    int i, free_cpus, num_add = 0;
    int* receive_domains;

    free_cpus = RM_XL_get_host_cpu() - (RM_RESSOURCE_MODEL_get_used_cpus() - alloc_summary.cpu_reduce);
    receive_domains = malloc(num_domains * sizeof(int));
    memset(receive_domains, -1, num_domains * sizeof(int));

    // Order domains that want more CPUs by their load and remove CPUs from domains that want to reduce CPUs
    for(i = 0; i < num_domains; i++)
    {
        if(alloc_ask[dom_list[i].domid].cpu_ask < 0)
        {
            RM_XL_change_vcpu(dom_list[i].domid, alloc_ask[dom_list[i].domid].cpu_ask);
        }
        else if(alloc_ask[dom_list[i].domid].cpu_ask > 0)
        {
            int j;
            
            for(j = num_add; (j >= 0 && RM_RESSOURCE_MODEL_get_domain_cpuload(receive_domains[j]) < dom_load[dom_list[i].domid].cpu_load); j--)
            {
                receive_domains[j + 1] = receive_domains[j];
            }

            receive_domains[j + 1] = dom_list[i].domid;
            num_add++;
        }
    }
    
    // Give all free CPUs to domains
    //for(i = 0; i < free_cpus; i++)
    for(i = 0; (i < alloc_summary.cpu_add && RM_RESSOURCE_MODEL_get_domain_cpuload(receive_domains[i]) > 90); i++)
    {
        if(free_cpus > 0)
        {
            RM_XL_change_vcpu(receive_domains[i], 1);
        }



        /*if(receive_domains[i] > -1)
        {
            RM_XL_change_vcpu(receive_domains[i], alloc_ask[receive_domains[i]].cpu_ask);
            syslog(LOG_NOTICE, "ADDED CPU TO: %d\n", receive_domains[i]);
        }*/
    }
    
    free(receive_domains);
    return 0;
}

// TODO add domain priorities
static int RM_ALLOCATOR_resolve_mem_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains)
{
    int i, free_mem, num_add = 0;
    int* receive_domains;

    free_mem = (RM_XL_get_host_mem_total() - (RM_RESSOURCE_MODEL_get_used_memory() - alloc_summary.mem_reduce)) / MEM_STEP;
    receive_domains = malloc(num_domains * sizeof(int));
    memset(receive_domains, -1, num_domains * sizeof(int));

    // Order domains that want more MEM by their load and remove MEM from domains that want to reduce MEM
    for(i = 0; i < num_domains; i++)
    {
        if(alloc_ask[dom_list[i].domid].mem_ask < 0)
        {
            RM_XL_change_memory(dom_list[i].domid, alloc_ask[dom_list[i].domid].mem_ask);
        }
        else if(alloc_ask[dom_list[i].domid].mem_ask > 0)
        {
            int j;
            
            for(j = num_add; (j >= 0 && RM_RESSOURCE_MODEL_get_domain_memload(receive_domains[j]) < dom_load[dom_list[i].domid].mem_load); j--)
            {
                    receive_domains[j + 1] = receive_domains[j];
            }
            
            receive_domains[j + 1] = dom_list[i].domid;
            num_add++;
        }
    }

    // Give all available MEM to domains that want more
    for(i = 0; i < free_mem; i++)
    {
        if(receive_domains[i] > -1)
        {
            RM_XL_change_memory(receive_domains[i], alloc_ask[receive_domains[i]].mem_ask);
            syslog(LOG_NOTICE, "ADDED MEMORY TO: %d\n", receive_domains[i]);
        }
    }

    free(receive_domains);
    return 0;
}

