#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // just for sleep()

#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <rm_xenstore.h>
#include <rm_xl.h>
#include <rm_ressource_model.h>
#include <rm_allocator.h>
#include <rm_numa_manager.h>

int compare_domains_by_cpuload(const void* a, const void* b)
{
    libxl_dominfo* first = (libxl_dominfo*) a;
    libxl_dominfo* sec = (libxl_dominfo*) b;
    double first_load = RM_RESSOURCE_MODEL_get_domain_cpuload(first->domid);
    double sec_load = RM_RESSOURCE_MODEL_get_domain_cpuload(sec->domid);
 
    if(first->vcpu_online == 1) return 1;
    else if(sec->vcpu_online == 1) return -1; 
    else return first_load - sec_load;
}

int compare_domains_by_memload(const void* a, const void* b)
{
    libxl_dominfo* first = (libxl_dominfo*) a;
    libxl_dominfo* sec = (libxl_dominfo*) b;
    double first_load = RM_RESSOURCE_MODEL_get_domain_memload(first->domid);
    double sec_load = RM_RESSOURCE_MODEL_get_domain_memload(sec->domid);
    int64_t first_mem = first->current_memkb + first->outstanding_memkb;
    int64_t sec_mem = sec->current_memkb + sec->outstanding_memkb;

    if(first_mem - MEM_STEP < DOMAIN_MIN_MEM) return 1;
    else if(sec_mem - MEM_STEP < DOMAIN_MIN_MEM) return -1;
    else return sec_load - first_load;
}

/**
 * Initialize daemon by killing parent, opening the log and closing standard output
 */
int init_daemon(void)
{
    pid_t pid;

    pid = fork();
    if(pid < 0)
        exit(EXIT_FAILURE);
    
    if(pid > 0)
        exit(EXIT_SUCCESS);
    
    umask(0);

    if(setsid < 0)
        exit(EXIT_FAILURE);

    if((chdir("/")) < 0)
        exit(EXIT_FAILURE);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    openlog("xen_ressource_manager", LOG_PID | LOG_NDELAY, LOG_MAIL);
    
    return 0;
}

/**
 * Initialize handles to use the accessors for the xen libraries
 */
int init_handle(void)
{
    if(RM_XL_init() < 0)
        return -1;

    if(RM_XENSTORE_init() < 0)
        return -1;

    if(RM_RESSOURCE_MODEL_init() < 0)
        return -1;

    if(RM_NUMA_MANAGER_init_numa_info() < 0)
        return -1;

    return 0;
}

/**
 * Close handles of all xen toolstack related contexts and the log
 */
void close_handle(void)
{
    RM_NUMA_MANAGER_close();
    RM_RESSOURCE_MODEL_free();
    RM_XENSTORE_close();
    RM_XL_close();

    closelog();
}

int main_ressource_manager(void)
{
    int num_domains, num_entries, i;
    libxl_dominfo* dom_list;
    libxl_dominfo* s_dom_list;
    domain_load_t* domain_load;
    
    dom_list = RM_XL_get_domain_list(&num_domains);
    if(dom_list == NULL)
        return -1;
    
    s_dom_list = malloc(num_domains * sizeof(libxl_dominfo));
    memcpy(s_dom_list, dom_list, num_domains * sizeof(libxl_dominfo));

    RM_RESSOURCE_MODEL_update(dom_list, num_domains);
    domain_load = RM_RESSOURCE_MODEL_get_ressource_data(&num_entries);
 
    // Check if used_cpus > host_cpus and adjust if necessary and possible
    if(RM_RESSOURCE_MODEL_get_used_cpus() > RM_XL_get_host_cpu())
    {
        int oversize = RM_RESSOURCE_MODEL_get_used_cpus() - RM_XL_get_host_cpu();

        qsort(s_dom_list, num_domains, sizeof(libxl_dominfo), compare_domains_by_cpuload);

        for(i = 0; i < oversize; i++)
        {
            int vcpu_removed = RM_XL_remove_vcpu(s_dom_list[i].domid);
            if(vcpu_removed > -1)
            {
                domain_load[s_dom_list[i].domid].vcpu_used -= 1;
                domain_load[s_dom_list[i].domid].vcpu_info[vcpu_removed].online = false;
            }
        }
    }
    // Check if used_mem > host_mem and adjust if necessary and possible
    if(RM_RESSOURCE_MODEL_get_used_memory() > RM_XL_get_host_mem_total())
    {
        int64_t used = RM_RESSOURCE_MODEL_get_used_memory(), total = RM_XL_get_host_mem_total();
        int rest = (used - total) % MEM_STEP;
        int oversize = (used - total) / MEM_STEP;
        
        if(rest > 0) oversize++;

        qsort(s_dom_list, num_domains, sizeof(libxl_dominfo), compare_domains_by_memload);

        for(i = 0; i < oversize; i++)
        {
            RM_XL_change_memory(s_dom_list[i].domid, -1 * MEM_STEP);
        }
    }

    syslog(LOG_NOTICE, "num_domains: %d; num_entries: %d\n", num_domains, num_entries);
    // Allocation asks and adjustments
    if(num_domains <= num_entries)
    {
        for(i = 0; i < num_domains; i++)
        {
            if(domain_load[dom_list[i].domid].dom_id >= 0)
                RM_ALLOCATOR_allocation_ask(&domain_load[dom_list[i].domid], dom_list[i]);
        }

        if(RM_ALLOCATOR_ressource_adjustment(dom_list, domain_load, num_domains) == 0)
            RM_NUMA_MANAGER_update_vcpu_placing(dom_list, s_dom_list, domain_load, num_domains);
    }

    syslog(LOG_NOTICE, "\n");

    libxl_dominfo_list_free(dom_list, num_domains);

    return 0;
}

int main(void)
{

    if(init_daemon() < 0)
        exit(EXIT_FAILURE);

    if(init_handle() < 0)
        exit(EXIT_FAILURE);

    while(1)
    {
        main_ressource_manager();
        sleep(5);
    }

    close_handle();
}

