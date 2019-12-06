#include <rm_allocator.h>

#include <stdio.h>
#include <rm_xl.h>

static int RM_ALLOCATOR_resolve_cpu_allocations(int* domid_list, int num_domains);

static int RM_ALLOCATOR_resolve_mem_allocations(int* domid_list, int  num_domains);

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
    printf("ID: %d; CPU: %f; MEM: %f\n", domain->dom_id, domain->cpu_load, domain->mem_load);
   
    if(domain->dom_id >= num_domains)
    {
        alloc_ask = realloc(alloc_ask, (domain->dom_id + 1) * sizeof(allocation_ask_t));
        if(alloc_ask == NULL)
            return -1;
    }
    
    // TODO only reduce if cpu|mem after change > 0 (not importatn, check in rm_xl functions)

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
    // TODO add if(cpu_add - cpu_reduce < total_host_cpu - host_cpu_used)
    if((alloc_summary.cpu_reduce >= alloc_summary.cpu_add) || 
        (alloc_summary.cpu_add - alloc_summary.cpu_reduce < RM_XL_get_host_cpu() - RM_XL_get_host_cpu_usage()))
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
        RM_ALLOCATOR_resolve_cpu_allocations(domid_list, num_domains);
    }

    // Resolve MEM allocation
    // TODO add if(mem_add - mem_reduce < host_mem_total - host_mem_used)
    if((alloc_summary.mem_reduce >= alloc_summary.mem_add) || 
        (alloc_summary.mem_add - alloc_summary.mem_reduce < RM_XL_get_host_mem_total() - RM_XL_get_host_mem_usage()))
    {
        for(i = 0; i < num_domains; i++)
        {
            if(alloc_ask[domid_list[i]].mem_ask != 0) 
                RM_XL_change_memory(domid_list[i], 100000 * alloc_ask[domid_list[i]].mem_ask); 
        }
    }
    else
    {
        RM_ALLOCATOR_resolve_mem_allocations(domid_list, num_domains);
    }

    return 0;
}

static int RM_ALLOCATOR_resolve_cpu_allocations(int* domid_list, int num_domains)
{
    // TODO
    return 0;
}

static int RM_ALLOCATOR_resolve_mem_allocations(int* domid_list, int num_domains)
{
    // TODO
    return 0;
}

