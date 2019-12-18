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
    syslog(LOG_NOTICE, "Host MEM: %ld\n", RM_XL_get_host_mem_total());
    syslog(LOG_NOTICE, "Used MEM: %ld\n", RM_RESSOURCE_MODEL_get_used_memory());
    syslog(LOG_NOTICE, "alloc_summary.cpu_add: %d\n", alloc_summary.cpu_add);
    syslog(LOG_NOTICE, "alloc_summary.cpu_reduce: %d\n", alloc_summary.cpu_reduce);

    if(num_domains <= 0 || (alloc_summary.cpu_add == 0 && alloc_summary.cpu_reduce == 0))
        return -1;
    
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
    int i, free_cpus, num_add = 0, num_standby = 0, standby_marker = 0;
    int* receive_domains;
    int* standby_domains;

    free_cpus = RM_XL_get_host_cpu() - (RM_RESSOURCE_MODEL_get_used_cpus() - alloc_summary.cpu_reduce);
    receive_domains = malloc(num_domains * sizeof(int));
    memset(receive_domains, -1, num_domains * sizeof(int));

    if(free_cpus < alloc_summary.cpu_add)
    {
        standby_domains = malloc(num_domains * sizeof(int));
        memset(standby_domains, -1, num_domains * sizeof(int));
        //standby_domains = malloc((num_domains - (alloc_summary.cpu_add + alloc_summary.cpu_reduce)) * sizeof(int));
        //memset(standby_domains, -1, (num_domains - (alloc_summary.cpu_add + alloc_summary.cpu_reduce)) * sizeof(int));
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
        else if(RM_RESSOURCE_MODEL_get_domain_cpuload(receive_domains[i]) > 90 && standby_domains != NULL)
        {
            // If more domains with higher load than 90 percent than free cpus availabe
            int j;
            for(j = standby_marker; j < num_standby && standby_domains[j] >= 0; j++)
            {
                if(RM_RESSOURCE_MODEL_get_domain_cpuload(standby_domains[j]) < 65)
                {
                    standby_marker = j;
                    RM_XL_change_vcpu(standby_domains[j], -1);
                    RM_XL_change_vcpu(receive_domains[i], 1);
                    syslog(LOG_NOTICE, "ADDED CPU to: %d; Using Cpu from domain: %d\n", receive_domains[i], standby_domains[j]);
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

// TODO add domain priorities
static int RM_ALLOCATOR_resolve_mem_allocations(libxl_dominfo* dom_list, domain_load_t* dom_load, int num_domains)
{
    int i, free_mem, domain_mem, num_add = 0, num_standby = 0, standby_marker = 0;
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
        else if(alloc_ask[dom_list[i].domid].mem_ask == 0 && domain_mem - MEM_STEP >= MIN_DOMAIN_MEM && free_mem < alloc_summary.mem_add)
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
        }
        else if(RM_RESSOURCE_MODEL_get_domain_memload(receive_domains[i] > 90 && standby_domains != NULL))
        {
            int j; 

            for(j = standby_marker; j > num_standby && standby_domains[j] >= 0; j++)
            {
                if(RM_RESSOURCE_MODEL_get_domain_memload(standby_domains[j] < 75))
                {
                    standby_marker = j;
                    RM_XL_change_memory(standby_domains[j], -MEM_STEP);
                    RM_XL_change_memory(receive_domains[i], MEM_STEP);
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

