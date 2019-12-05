#include <rm_allocator.h>

#include <stdio.h>
int RM_ALLOCATOR_allocation_ask(domain_load_t* domain)
{
    // TODO 
    printf("ID: %d; CPU: %f; MEM: %f; Iterations: %ld\n", domain->dom_id, domain->cpu_load, domain->mem_load, domain->iterations);
    return 0;
}

