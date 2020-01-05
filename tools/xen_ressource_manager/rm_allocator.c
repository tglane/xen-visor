#include <rm_allocator.h>

#include <syslog.h>
#include <rm_xl.h>

static int RM_ALLOCATOR_resolve_cpu_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains);

static int RM_ALLOCATOR_resolve_mem_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains);

/**
 * Struct containing memory and cpu asks of one domain
 */
struct allocation_ask {
    int cpu_ask;
    int mem_ask;
};
typedef struct allocation_ask allocation_ask_t;

/**
 * Struct containing summary of cpu and memory add and reduce asks
 */
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

static int compare_domains_by_cpuload(const void* a, const void* b)
{
    double first_load = RM_RESSOURCE_MODEL_get_domain_cpuload(*(int*) a), sec_load = RM_RESSOURCE_MODEL_get_domain_cpuload(*(int*) b);

    if(first_load == -1)
        return 1;
    else if(sec_load == -1)
        return -1;
    else
        return first_load - sec_load;
}

static int compare_domains_by_memload(const void* a, const void* b)
{
    double first_load = RM_RESSOURCE_MODEL_get_domain_memload(*(int*) a), sec_load = RM_RESSOURCE_MODEL_get_domain_memload(*(int*) b);

    if(first_load == -1)
        return 1;
    else if(sec_load == -1)
        return -1;
    else
        return first_load - sec_load;    
}

int RM_ALLOCATOR_allocation_ask(domain_load_t* domain, libxl_dominfo dom_info)
{
    int64_t domain_memory;

    syslog(LOG_NOTICE, "ID: %d; CPU: %f; MEM: %f\n", domain->dom_id, domain->cpu_load, domain->mem_load);
    syslog(LOG_NOTICE, "ID: %d; Count CPUs: %d; Count MEM: %ld\n", dom_info.domid, dom_info.vcpu_online, dom_info.current_memkb + dom_info.outstanding_memkb);

    if(domain->dom_id >= num_domains)
    {
        alloc_ask = realloc(alloc_ask, (domain->dom_id + 1) * sizeof(allocation_ask_t));
        if(alloc_ask == NULL)
            return -1;
    }
    
    // Ressource allocation ask for vcpus
    if(domain->cpu_load > 80 && (dom_info.vcpu_online + 1 <= RM_XL_get_host_cpu()))
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
    if(domain->mem_load > 80 && ((domain_memory + MEM_STEP) <= RM_XL_get_host_mem_total()))
    {
        alloc_ask[domain->dom_id].mem_ask = 1;
        alloc_summary.mem_add++; 
    }
    else if(domain->mem_load < 40 && ((domain_memory - MEM_STEP) > DOMAIN_MIN_MEM))
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
    syslog(LOG_NOTICE, "Host MEM: %ld\n", RM_XL_get_host_mem_total());
    syslog(LOG_NOTICE, "Used MEM: %ld\n", RM_RESSOURCE_MODEL_get_used_memory());
    syslog(LOG_NOTICE, "alloc_summary.cpu_add: %d\n", alloc_summary.cpu_add);
    syslog(LOG_NOTICE, "alloc_summary.cpu_reduce: %d\n", alloc_summary.cpu_reduce);
    syslog(LOG_NOTICE, "alloc_summary.mem_add: %d\n", alloc_summary.mem_add);
    syslog(LOG_NOTICE, "alloc_summary.mem_reduce: %d\n", alloc_summary.mem_reduce);

    if(num_domains <= 0 || (alloc_summary.cpu_add == 0 && alloc_summary.cpu_reduce == 0
         && alloc_summary.mem_add == 0 && alloc_summary.mem_reduce == 0))
        return -1;
    
    // Resolve CPU allocations
    if((alloc_summary.cpu_add > 0 || alloc_summary.cpu_reduce > 0) &&
        ((alloc_summary.cpu_reduce >= alloc_summary.cpu_add) || 
        (alloc_summary.cpu_add - alloc_summary.cpu_reduce <= RM_XL_get_host_cpu() - RM_RESSOURCE_MODEL_get_used_cpus())))
    {
        // Resolving all allocation asks is possible
        for(i = 0; i < num_domains; i++)
        {
            if(alloc_ask[dom_list[i].domid].cpu_ask != 0)
            {
                syslog(LOG_NOTICE, "direct CPU_SET for id: %d\n", dom_list[i].domid);
                RM_XL_change_vcpu(dom_list[i].domid, alloc_ask[dom_list[i].domid].cpu_ask);
            }
        }
    }
    else if(alloc_summary.cpu_add > 0 || alloc_summary.cpu_reduce > 0)
    {
        RM_ALLOCATOR_resolve_cpu_allocations(dom_list, dom_load, num_domains);
    }

    // Resolve MEM allocation
    if((alloc_summary.mem_add > 0 || alloc_summary.mem_reduce > 0) &&
        ((alloc_summary.mem_reduce >= alloc_summary.mem_add) || 
        ((alloc_summary.mem_add - alloc_summary.mem_reduce) * MEM_STEP <= RM_XL_get_host_mem_total() - RM_RESSOURCE_MODEL_get_used_memory())))
    {
        // Resolving all allocation asks is possible
        for(i = 0; i < num_domains; i++)
        {
            if(alloc_ask[dom_list[i].domid].mem_ask != 0)
            { 
                syslog(LOG_NOTICE, "direct MEM_SET for id: %d with %d\n", dom_list[i].domid, MEM_STEP * alloc_ask[dom_list[i].domid].mem_ask);
                RM_XL_change_memory(dom_list[i].domid, MEM_STEP * alloc_ask[dom_list[i].domid].mem_ask);
            }
        }
    }
    else if(alloc_summary.mem_add > 0 || alloc_summary.mem_reduce > 0)
    {
        RM_ALLOCATOR_resolve_mem_allocations(dom_list, dom_load, num_domains);
    }

    alloc_summary = (allocation_summary_t) {0, 0, 0, 0};
    return 0;
}

/**
 * Resolves the cpu allocation distribution when domains want to allocate more cpus than free
 * Priorizing domains with higher cpu load
 *
 * Reduces the cpu allocation of domains that want to reduce it
 * If not enough cpus are free to fulfill all allocation asks and there are domains with very high load (>90%)
 * the allocation of domains with mid load (50>x>20) is reduced to give it to domains with very high load
 * -> It is not always possible to fulfill all allocation asks so high load domains are priorized
 *
 * Returns 0 if operation was successful and -1 else
 * Parameter dom_list array of libxl_dominfo with length num_domains
 * Parameter dom_load array of domain_load_t with length num_domains
 * Parameter num_domains number of domains in the arrays
 */ 
static int RM_ALLOCATOR_resolve_cpu_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains)
{
    int i, free_cpus, num_add = 0, num_standby = 0;
    int* receive_domains;
    int* standby_domains;

    free_cpus = RM_XL_get_host_cpu() - (RM_RESSOURCE_MODEL_get_used_cpus() - alloc_summary.cpu_reduce);
    receive_domains = malloc(num_domains * sizeof(int));
    memset(receive_domains, -1, num_domains * sizeof(int));

    if(free_cpus < alloc_summary.cpu_add)
    {
        standby_domains = malloc(num_domains * sizeof(int));
        memset(standby_domains, -1, num_domains * sizeof(int));
    }

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
        else if(alloc_ask[dom_list[i].domid].cpu_ask == 0 && dom_list[i].vcpu_online > 1 && free_cpus < alloc_summary.cpu_add)
        {
            int j;

            for(j = num_standby; (j >= 0 && (RM_RESSOURCE_MODEL_get_domain_cpuload(standby_domains[j]) > dom_load[dom_list[i].domid].cpu_load || RM_RESSOURCE_MODEL_get_domain_cpuload(standby_domains[j]) == -1)); j--)
            {
                standby_domains[j + 1] = standby_domains[j];
            }

            standby_domains[j + 1] = dom_list[i].domid;
            num_standby++;
        }
    }

    syslog(LOG_NOTICE, "###################################### cpu standby_domains:\n");
    for(i = 0; i < num_standby; i++)
        syslog(LOG_NOTICE, "id: %d\n", standby_domains[i]);
    syslog(LOG_NOTICE, "######################################\n");
    
    // Give all free CPUs to domains
    for(i = 0; i < alloc_summary.cpu_add; i++)
    {
        if(free_cpus > 0)
        {
            RM_XL_change_vcpu(receive_domains[i], alloc_ask[receive_domains[i]].cpu_ask);
            syslog(LOG_NOTICE, "ADDED CPU TO: %d; Using a free cpu\n", receive_domains[i]);
            free_cpus--;
        }
        else if(RM_RESSOURCE_MODEL_get_domain_cpuload(receive_domains[i]) > (105 - (RM_RESSOURCE_MODEL_get_domain_priority(receive_domains[i]) * 5))
                && standby_domains != NULL)
        {
            // If more domains with higher load than 90 percent than free cpus availabe
            int j;

            for(j = 0; j < num_standby && standby_domains[j] >= 0; j++)
            {
                if(RM_RESSOURCE_MODEL_get_domain_cpuload(standby_domains[j]) < 50 &&
                        RM_RESSOURCE_MODEL_get_domain_priority(receive_domains[i]) >= RM_RESSOURCE_MODEL_get_domain_priority(standby_domains[j]))
                {
                    if(RM_XL_change_vcpu(standby_domains[j], -1) == 0)
                    {
                        if(RM_XL_change_vcpu(receive_domains[i], 1) == 0)
                            syslog(LOG_NOTICE, "ADDED CPU to: %d; Using Cpu from domain: %d\n", receive_domains[i], standby_domains[j]);
                        
                        standby_domains[j] = -1;
                        qsort(standby_domains, num_standby, sizeof(int), compare_domains_by_cpuload);
                    }
                    break;
                }
            }
        }
    }
    
    free(receive_domains);
    if(standby_domains != NULL)
        free(standby_domains);
    return 0;
}

/**
 * Resolves the memory allocation distribution when domains want to allocate more memory than free
 * Priorizing domains with higher memory load
 *
 * Reduces the memory allocation of domains that want to reduce it first
 * If not enough memory is free to fulfill all allocation asks after that and there are domains with very high load (>90%)
 * the allocation of domains with mid load (75>x>20) is reduced to give it to domains with very high load
 * -> It is not always possible to fulfill all allocation asks so high load domains are priorized
 *
 * Returns 0 if operation was successful and -1 else
 * Parameter dom_list array of libxl_dominfo with length num_domains
 * Parameter dom_load array of domain_load_t with length num_domains
 * Parameter num_domains number of domains in the arrays
 */ 
static int RM_ALLOCATOR_resolve_mem_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains)
{
    int i, free_mem, domain_mem, num_add = 0, num_standby = 0;
    int* receive_domains;
    int* standby_domains;

    free_mem = (RM_XL_get_host_mem_total() - (RM_RESSOURCE_MODEL_get_used_memory() - alloc_summary.mem_reduce)) / MEM_STEP;
    receive_domains = malloc(num_domains * sizeof(int));
    memset(receive_domains, -1, num_domains * sizeof(int));

    if(free_mem < alloc_summary.mem_add)
    {
        standby_domains = malloc(num_domains * sizeof(int));
        memset(standby_domains, -1, num_domains * sizeof(int));
    } 

    // Order domains that want more MEM by their load and remove MEM from domains that want to reduce MEM
    for(i = 0; i < num_domains; i++)
    {
        domain_mem = dom_list[i].current_memkb + dom_list[i].outstanding_memkb;

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
        else if(alloc_ask[dom_list[i].domid].mem_ask == 0 && domain_mem - MEM_STEP >= DOMAIN_MIN_MEM && free_mem < alloc_summary.mem_add)
        {
            int j;
            
            for(j = num_standby; (j >= 0 && (RM_RESSOURCE_MODEL_get_domain_memload(standby_domains[j]) > dom_load[dom_list[i].domid].mem_load || RM_RESSOURCE_MODEL_get_domain_memload(standby_domains[j]) == -1)); j--)
            {
                standby_domains[j + 1] = standby_domains[j];
            }

            standby_domains[j + 1] = dom_list[i].domid;
            num_standby++;
        }
    }

    syslog(LOG_NOTICE, "###################################### mem standby_domains:\n");
    for(i = 0; i < num_standby; i++)
        syslog(LOG_NOTICE, "id: %d\n", standby_domains[i]);
    syslog(LOG_NOTICE, "######################################\n");

    // Give all available MEM to domains that want more
    for(i = 0; i < alloc_summary.mem_add; i++)
    {
        if(free_mem > 0)
        {
            RM_XL_change_memory(receive_domains[i], MEM_STEP);
            free_mem--;
            syslog(LOG_NOTICE, "ADDED MEM TO: %d; Using free mem\n", receive_domains[i]);
        }
        else if(RM_RESSOURCE_MODEL_get_domain_memload(receive_domains[i]) > (105 - (RM_RESSOURCE_MODEL_get_domain_priority(receive_domains[i]) * 5))
                && standby_domains != NULL)
        {
            int j; 

            for(j = 0; j < num_standby && standby_domains[j] >= 0; j++)
            {
                if(RM_RESSOURCE_MODEL_get_domain_memload(standby_domains[j]) < 75 &&
                        RM_RESSOURCE_MODEL_get_domain_priority(receive_domains[i]) >= RM_RESSOURCE_MODEL_get_domain_priority(standby_domains[j]))
                {
                    if(RM_XL_change_memory(standby_domains[j], -MEM_STEP) == 0)
                    {
                        if(RM_XL_change_memory(receive_domains[i], MEM_STEP) == 0)
                            syslog(LOG_NOTICE, "ADDED MEM to: %d; Using mem from domain: %d\n", receive_domains[i], standby_domains[j]);
                        
                        standby_domains[j] = -1;
                        qsort(standby_domains, num_standby, sizeof(int), compare_domains_by_memload);
                    }
                    break;
                }
            }
        }
    }

    free(receive_domains);
    if(standby_domains != NULL)
        free(standby_domains);
    return 0;
}

