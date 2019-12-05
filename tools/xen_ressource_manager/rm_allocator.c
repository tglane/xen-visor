#include <rm_allocator.h>

#include <stdio.h>
#include <rm_xl.h>

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

int RM_ALLOCATOR_allocation_ask(domain_load_t* domain)
{
    printf("ID: %d; CPU: %f; MEM: %f; Iterations: %ld\n", domain->dom_id, domain->cpu_load, domain->mem_load, domain->iterations);
   
    if(domain->dom_id >= num_domains)
    {
        alloc_ask = realloc(alloc_ask, (domain->dom_id + 1) * sizeof(allocation_ask_t));
        if(alloc_ask == NULL)
            return -1;
    }
    
    // TODO only reduce if cpu|mem after change > 0

    // Ressource allocation ask for vcpus
    if(domain->cpu_load > 80)
    { 
        alloc_ask[domain->dom_id].cpu_ask = 1;
        alloc_summary.cpu_add++;
    }
    else if(domain->cpu_load < 20)
    {
        alloc_ask[domain->dom_id].cpu_ask = -1;
        alloc_summary.cpu_reduce++;
    }
    else 
    {
        alloc_ask[domain->dom_id].cpu_ask = 0;
    }

    // Ressource allocation ask for memory
    if(domain->mem_load > 90)
    {
        alloc_ask[domain->dom_id].mem_ask = 1;
        alloc_summary.mem_add++; 
    }
    else if(domain->mem_load < 50)
    {
        alloc_ask[domain->dom_id].mem_ask = -1;
        alloc_summary.mem_reduce++;
    }
    else
    {
        alloc_ask[domain->dom_id].mem_ask = 0;
    }
    
    printf("ID: %d; CPU_ASK: %d; MEM_ASK: %d\n", domain->dom_id, alloc_ask[domain->dom_id].cpu_ask, alloc_ask[domain->dom_id].mem_ask);
    
    return 0;
}

int RM_ALLOCATOR_ressource_adjustment(int* domid_list, int num_domains)
{
    int i;

    // Resolve CPU allocations
    if((alloc_summary.cpu_reduce >= alloc_summary.cpu_add) || 
        (alloc_summary.cpu_add - alloc_summary.cpu_reduce < RM_XL_get_host_cpu()))
    {
        // Resolve all allocation asks
        for(i = 0; i < num_domains; i++)
        {
            if(alloc_ask[domid_list[i]].cpu_ask != 0) 
                RM_XL_change_vcpu(domid_list[i], alloc_ask[domid_list[i]].cpu_ask);
        }
    }
    else
    {
        // TODO
    }

    // Resolve MEM allocation
    if((alloc_summary.mem_reduce >= alloc_summary.mem_add) || 
        (alloc_summary.mem_add - alloc_summary.mem_reduce < RM_XL_get_host_mem_total()))
    {
        for(i = 0; i < num_domains; i++)
        {
            if(alloc_ask[domid_list[i]].mem_ask != 0) 
                RM_XL_change_memory(domid_list[i], 100000 * alloc_ask[domid_list[i]].mem_ask); 
        }
    }
    else
    {
        // TODO
    }

    return 0;
}

