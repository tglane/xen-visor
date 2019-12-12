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

int init_daemon(void)
{
    pid_t pid;

    // Init daemon
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

int init_handle(void)
{
    // Init handles
    if(RM_XL_init() < 0)
        return -1;

    if(RM_XENSTORE_init() < 0)
        return -1;

    RM_RESSOURCE_MODEL_init();

    return 0;
}

int main_ressource_manager(void)
{
    int num_domains, num_entries, i;
    libxl_dominfo* dom_list;
    domain_load_t* domain_load;
    
    dom_list = RM_XL_get_domain_list(&num_domains);
    if(dom_list == NULL)
        return -1;
    
    RM_RESSOURCE_MODEL_update(dom_list, num_domains);
    domain_load = RM_RESSOURCE_MODEL_get_ressource_data(&num_entries);
  
    //printf("num_domains: %d; num_entries: %d\n", num_domains, num_entries); 
    syslog(LOG_NOTICE, "num_domains: %d; num_entries: %d\n", num_domains, num_entries);
    if(num_domains <= num_entries)
    {
        // Get ressource allocation asks from all domains
        for(i = 0; i < num_domains; i++)
        {
            if(domain_load[dom_list[i].domid].dom_id >= 0)
            {
                RM_ALLOCATOR_allocation_ask(&domain_load[dom_list[i].domid], dom_list[i]);
            }
        }
    }

    RM_ALLOCATOR_ressource_adjustment(dom_list, domain_load, num_domains);    

    syslog(LOG_NOTICE, "\n");

    libxl_dominfo_list_free(dom_list, num_domains);

    return 0;
}

int main(void)
{
    int i = 0;

    if(init_handle() < 0 || init_daemon() < 0)
        exit(EXIT_FAILURE);

    while(1)
    {
        // TODO 
        main_ressource_manager();
        i++;
        sleep(5);
    }

    RM_RESSOURCE_MODEL_free();
    RM_XENSTORE_close();
    RM_XL_close();

    closelog();
}

