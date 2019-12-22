#include <rm_numa_manager.h>

static int compare_domains_by_vcpucount(const void* a, const void* b)
{
    libxl_dominfo* first = (libxl_dominfo*) a;
    libxl_dominfo* sec = (libxl_dominfo*) b;

    return sec->vcpu_online - first->vcpu_online;
} 

int RM_NUMA_MANAGER_update_vcpu_placing(libxl_dominfo* dom_list, libxl_dominfo* s_dom_list, int num_domains)
{
    int i, j, num_cpu;
    libxl_cputopology* cpu_top = RM_XL_get_cpu_topology(&num_cpu);
    
    if(cpu_top == NULL)
        return -1;
    
    qsort(s_dom_list, num_domains, sizeof(libxl_dominfo), compare_domains_by_vcpucount);

    // TODO get current cpu topology and put domains vCPUs clever on pCPUs
    for(i = 0; i < num_domains; i++)
    {
        int num_vcpus;
        libxl_vcpuinfo* vcpu_info;

        vcpu_info = RM_XL_get_vcpu_list(s_dom_list[i].domid, &num_vcpus);
        if(vcpu_info == NULL)
        {
            libxl_cputopology_list_free(cpu_top, num_cpu);
            return -1;
        }

        for(j = 0; j < num_vcpus; j++)
        {

            RM_XL_pin_vcpu(s_dom_list[i].domid, vcpu_info[j].vcpuid, i+j, VCPU_PIN_HARD);
        }

        libxl_vcpuinfo_list_free(vcpu_info, num_vcpus);
    }

    libxl_cputopology_list_free(cpu_top, num_cpu);
    return 0;
}

