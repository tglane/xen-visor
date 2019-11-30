#include <stdlib.h>
#include <stdio.h>

#include <rm_xenstore.h>
#include <rm_xl.h>
#include <rm_ressource_model.h>

int main_ressource_checker(void)
{
    int num_domains, num_entries, i;
    int* domid_list;
    domain_load_t* domain_load;

    domid_list = RM_XL_get_domain_list(&num_domains);
    if(domid_list == NULL)
        return -1;

    RM_RESSOURCE_MODEL_update(domid_list, num_domains);

    domain_load = RM_RESSOURCE_MODEL_get_adaption_domains(&num_entries);
    for(i = 0; i < num_entries; i++)
    {
        printf("ID: %d; MEM: %f; CPU: %f\n", domain_load[i].dom_id, domain_load[i].mem_load, domain_load[i].cpu_load);
    }

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

int initHandle(void)
{
    if(RM_XL_init() < 0)
        return -1;

    if(RM_XENSTORE_init() < 0)
        return -1;

    return 0;
}

int main(void)
{
    if(initHandle() < 0)
        exit(0);

    main_ressource_checker();

    RM_XENSTORE_close();
    RM_XL_close();
}

