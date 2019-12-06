#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // just for sleep()

#include <rm_xenstore.h>
#include <rm_xl.h>
#include <rm_ressource_model.h>
#include <rm_allocator.h>

int initHandle(void)
{
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
    int* domid_list;
    domain_load_t* domain_load;
    
    domid_list = RM_XL_get_domain_list(&num_domains);
    if(domid_list == NULL)
        return -1;
    
    RM_RESSOURCE_MODEL_update(domid_list, num_domains);
    domain_load = RM_RESSOURCE_MODEL_get_ressource_data(&num_entries);
  
    printf("num_domains: %d; num_entries: %d\n", num_domains, num_entries); 
    if(num_domains <= num_entries)
    {
        // Get the load data from all exciting domains
        for(i = 0; i < num_domains; i++)
        {
            if(domain_load[domid_list[i]].dom_id >= 0)
                RM_ALLOCATOR_allocation_ask(&domain_load[domid_list[i]]);
        }
    }

    RM_ALLOCATOR_ressource_adjustment(domid_list, num_domains);    

    printf("\n");

    // test cpu and ram change
    /*if(num_domains > 1)
    {
        if(RM_XL_change_vcpu(domid_list[1], 1) == 0)
            printf("added vcpu to domain 1\n");

        if(RM_XL_change_memory(domid_list[1], 100000) == 0)
            printf("added memory to domain 1\n");
    }*/

    free(domid_list);

    return 0;
}

int main(void)
{
    int i; 

    if(initHandle() < 0)
        exit(0);

    for(i = 0; i < 1000; i++)
    {
        main_ressource_manager();
        sleep(1);
    }

    RM_RESSOURCE_MODEL_free();
    RM_XENSTORE_close();
    RM_XL_close();
}

