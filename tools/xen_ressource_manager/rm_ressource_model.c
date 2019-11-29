#include <rm_ressource_model.h>

#include <stdio.h>
#include <rm_xenstore.h>

int RM_RESSOURCE_MODEL_update(int* domid_list, int num_domains)
{
    int i;
    double memload, cpuload;

    if(!RM_XENSTORE_initialized())
    {
        printf("xenstore not initialized\n");
        return -1;
    }

    for(i = 0; i < num_domains; i++)
    {
        memload = RM_XENSTORE_read_domain_memload(domid_list[i]);
        cpuload = RM_XENSTORE_read_domain_cpuload(domid_list[i]);

        printf("domain_id: %d, memload: %f, cpuload: %f\n", domid_list[i], memload, cpuload);
    }

    return 0;
}

