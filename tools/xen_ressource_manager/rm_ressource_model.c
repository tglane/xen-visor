#include <rm_ressource_model.h>

#include <stdlib.h>
#include <stdio.h>
#include <rm_xenstore.h>

static domain_load_t* ressource_data;
int max_domain_id = 0;

void RM_RESSOURCE_MODEL_init(void)
{
    ressource_data = malloc(sizeof(domain_load_t));
}

void RM_RESSOURCE_MODEL_free(void)
{
    free(ressource_data);
}

int RM_RESSOURCE_MODEL_update(int* domid_list, int num_domains)
{
    int i;
    double memload, cpuload;

    if(!RM_XENSTORE_initialized())
    {
        return -1;
    }

    for(i = 0; i < num_domains; i++)
    {
        // Realloc memory for ressource_data if max_domain_id is lower than current id
        if(domid_list[i] > max_domain_id)
        {
            ressource_data = realloc(ressource_data, domid_list[i]);
            if(ressource_data == NULL)
                return -1;
            max_domain_id = domid_list[i];
        }

        memload = RM_XENSTORE_read_domain_memload(domid_list[i]);
        cpuload = RM_XENSTORE_read_domain_cpuload(domid_list[i]);

        if(memload < 0 || cpuload < 0)
            return -1;

        // TODO save current domain load
        ressource_data[i].dom_id = domid_list[i];
        ressource_data[i].cpu_load = cpuload;
        ressource_data[i].mem_load = memload;

        printf("domain_id: %d, memload: %f, cpuload: %f\n", domid_list[i], memload, cpuload);
    }

    return 0;
}

domain_load_t* RM_RESSOURCE_MODEL_get_adaption_domains(int* num_entries)
{
    // TODO get all domains which need hardware adaption
    // TODO split to memory and cpu?
    domain_load_t* ret;
    ret = ressource_data;
    *num_entries = max_domain_id;
    return ret;
    
}

