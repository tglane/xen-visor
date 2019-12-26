#include <rm_numa_manager.h>

#include <syslog.h>

struct numa_node_info {
    int num_cpus;
    int num_free_cpus;
    int* free_cpu_id;
    int64_t size_mem;
    int64_t size_free_mem;
};
typedef struct numa_node_info numa_node_info_t;

static numa_node_info_t* node_info;

static int compare_domains_by_vcpucount(const void* a, const void* b)
{
    libxl_dominfo* first = (libxl_dominfo*) a;
    libxl_dominfo* sec = (libxl_dominfo*) b;

    return sec->vcpu_online - first->vcpu_online;
} 

static int compare_nodes_by_freecpus(const void* a, const void* b)
{
    numa_node_info_t* first = (numa_node_info_t*) a;
    numa_node_info_t* sec = (numa_node_info_t*) b;

    return sec->num_free_cpus - first->num_free_cpus;
}

static int get_free_pcpu(int node_id)
{
    int i;
    int* free_cpus;
    if(node_info == NULL || node_info[node_id].free_cpu_id == NULL)
        return -1;

    // TODO

    free_cpus = node_info[node_id].free_cpu_id;
    for(i = 0; i < node_info[node_id].num_cpus; i++)
    {
        if(free_cpus[i] > 0)
        {
            free_cpus[i] = 0;
            return i;
        }
    }

    return -1;
}

int RM_NUMA_MANAGER_init_numa_info(void)
{
    int i, num_cpu;
    libxl_physinfo* phys_info = RM_XL_get_physinfo();
    libxl_cputopology* cpu_top = RM_XL_get_cpu_topology(&num_cpu);

    if(node_info != NULL || phys_info == NULL || cpu_top == NULL)
        return -1;

    node_info = malloc(phys_info->nr_nodes * sizeof(numa_node_info_t));
    memset(node_info, 0, phys_info->nr_nodes * sizeof(numa_node_info_t));

    // TODO
    for(i = 0; i < num_cpu; i++)
    {
        node_info[cpu_top[i].node].num_cpus++;
    }

    libxl_cputopology_list_free(cpu_top, num_cpu);
    free(phys_info);
    return 0;
}

void RM_NUMA_MANAGER_close(void)
{
    if(node_info != NULL)
        free(node_info);
}

int RM_NUMA_MANAGER_update_vcpu_placing(libxl_dominfo* dom_list, libxl_dominfo* s_dom_list, int num_domains)
{
    int i, j, num_cpu, num_nodes;
    libxl_cputopology* cpu_top = RM_XL_get_cpu_topology(&num_cpu);
    libxl_numainfo* numa_info = RM_XL_get_numa_topology(&num_nodes);
    libxl_physinfo* phys_info = RM_XL_get_physinfo();

    if(node_info == NULL)
        return -1;

    // Reset free cpus and free mem of node_info struct array 
    for(i = 0; i < num_nodes; i++)
    {
        // TODO init node_info[i].free_cpus correctly with correct pCPU ids
        node_info[i].num_free_cpus = node_info[i].num_cpus;
        node_info[i].free_cpu_id = malloc(num_cpu * sizeof(int));
        memset(node_info[i].free_cpu_id, -1, num_cpu * sizeof(int));

        node_info[i].size_mem = numa_info[i].size;
        node_info[i].size_free_mem = numa_info[i].size;
    }
    for(i = 0; i < num_cpu; i++)
    {
        node_info[cpu_top[i].node].free_cpu_id[i] = 1;
    }

    /**
     * INFO:
     *
     * Cores per node : phys_info->cores_per_socket
     * 
     * # Nodes : phys_info->nr_nodes | num_nodes
     * # Cores : phys_info->nr_cpus  | num_cpu
     *
     * Node-id of Core : cpu_top[i].node
     * Socket-id of Core : cpu_top[i].socket
     * 
     * Memory size of Node : numa_info[i].size
     * Memory free of Node : numa_info[i].free (not usefuf; store own calculated value in node_info_t)
     *
     * # Domains : num_domains
     *
     * # vCPUs per Domain : dom_list[i].vcpu_online
     * Max vCPU id of Domain : dom_list[i].vcpu_max_id
     */


    if(cpu_top == NULL || phys_info == NULL || numa_info == NULL)
        return -1;

    // Sort domains by amount of active vcpus to place big domains first 
    qsort(s_dom_list, num_domains, sizeof(libxl_dominfo), compare_domains_by_vcpucount);
    
    // Place domains vCPUs on physical CPUs using a greedy approach
    for(i = 0; i < num_domains; i++)
    {
        int vcpus_placed = 0, num_vcpus;
        int64_t mem_placed = 0, domain_memory = s_dom_list[i].current_memkb + s_dom_list[i].outstanding_memkb;
        libxl_vcpuinfo* vcpu_info = RM_XL_get_vcpu_list(s_dom_list[i].domid, &num_vcpus);
        if(vcpu_info == NULL)
            return -1;

        // Sort nodes by free cpus so the node with most free cpus comes first
        qsort(node_info, num_nodes, sizeof(numa_node_info_t), compare_nodes_by_freecpus);

        for(j = 0; j < num_nodes && vcpus_placed < s_dom_list[i].vcpu_online && mem_placed < domain_memory; j++) 
        {
            // Only if all vCPUs and complete domain memory fits in one node, place all vCPUs on the node
            // Else split the domain and place at least one cpu with the rest of the memory on another node
            if(node_info[j].num_free_cpus >= s_dom_list[i].vcpu_online && node_info[j].size_free_mem >= domain_memory)
            {
                int k;

                // Place all domains vcpus and memory on the current node
                for(k = 0; k < num_vcpus; k++)
                {
                    if(vcpu_info[k].online)
                    {
                        int pcpu = get_free_pcpu(j);
                        if(RM_XL_pin_vcpu(s_dom_list[i].domid, vcpu_info[k].vcpuid, pcpu, VCPU_PIN_HARD))
                            return -1;

                        syslog(LOG_NOTICE, "[NUMA - Placement 1] Domain: %d; vCPU: %d; pCPU: %d\n", s_dom_list[i].domid, vcpu_info[k].vcpuid, pcpu);
                        node_info[j].num_free_cpus--;
                        vcpus_placed++;
                    }
                }

                node_info[j].size_free_mem -= domain_memory;
                mem_placed = domain_memory;
            }
            else if(node_info[j].num_free_cpus >= s_dom_list[i].vcpu_online && node_info[j].size_free_mem < domain_memory)
            {
                int k, l;

                // Place as much vcpus on the nodes cpus as possible except of one because of the memory and go to the next node
                for(k = 0, l = 0; k < num_vcpus && l < node_info[j].num_free_cpus - 1; k++)
                {
                    if(vcpu_info[k].online)
                    {
                        int pcpu = get_free_pcpu(j);
                        if(RM_XL_pin_vcpu(s_dom_list[i].domid, vcpu_info[k].vcpuid, pcpu, VCPU_PIN_HARD))
                            return -1;

                        syslog(LOG_NOTICE, "[NUMA - Placement 2] Domain: %d; vCPU: %d; pCPU: %d\n", s_dom_list[i].domid, vcpu_info[k].vcpuid, pcpu);
                        node_info[j].num_free_cpus--;
                        vcpus_placed++;

                        l++;
                    }
                }

                mem_placed = node_info[j].size_free_mem;
                node_info[j].num_free_cpus = 0;
            }
            else
            {
                int k, l;

                // Place as much vcpus and memory on the node as possible and go to the next node
                for(k = 0, l = 0; k < num_vcpus && l < node_info[j].num_free_cpus; k++)
                {
                    if(vcpu_info[k].online)
                    {
                        int pcpu = get_free_pcpu(j);
                        if(RM_XL_pin_vcpu(s_dom_list[i].domid, vcpu_info[k].vcpuid, pcpu, VCPU_PIN_HARD))
                            return -1;

                        syslog(LOG_NOTICE, "[NUMA - Placement 3] Domain: %d; vCPU: %d; pCPU: %d\n", s_dom_list[i].domid, vcpu_info[k].vcpuid, pcpu);
                        node_info[j].num_free_cpus--;
                        vcpus_placed++;

                        l++;
                    }
                }

                mem_placed = node_info[j].size_free_mem;
                node_info[j].size_free_mem = 0;
            }
        }

        libxl_vcpuinfo_list_free(vcpu_info, num_vcpus);
    }

    for(i = 0; i < num_nodes; i++)
        free(node_info[i].free_cpu_id);
    
    libxl_cputopology_list_free(cpu_top, num_cpu);
    libxl_numainfo_list_free(numa_info, num_nodes);
    free(phys_info);
    return 0;
}

