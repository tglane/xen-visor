#include <rm_ressource_model.h>

#include <stdlib.h>
#include <stdio.h>
#include <rm_xenstore.h>

static domain_load_t* ressource_data;
static int max_domain_id = 0;

void RM_RESSOURCE_MODEL_init(void)
{
    ressource_data = malloc(sizeof(domain_load_t));
    ressource_data[0] = (domain_load_t) {0, 0.0, 0.0, 0};
}

void RM_RESSOURCE_MODEL_free(void)
{
    free(ressource_data);
}

int RM_RESSOURCE_MODEL_update(int* domid_list, int num_domains)
{
    int i, j;
    double memload, cpuload;

    if(!RM_XENSTORE_initialized())
    {
        return -1;
    }

    for(i = 0; i < num_domains; i++)
    {
        memload = RM_XENSTORE_read_domain_memload(domid_list[i]);
        cpuload = RM_XENSTORE_read_domain_cpuload(domid_list[i]);

        if(memload < 0 || cpuload < 0)
            continue;

        // Realloc memory for ressource_data if max_domain_id is lower than current id
        if(domid_list[i] > max_domain_id)
        {
            ressource_data = realloc(ressource_data, (domid_list[i] + 1) * sizeof(domain_load_t));
            if(ressource_data == NULL)
                return -1;

            for(j = max_domain_id + 1; j < domid_list[i]; j++)
            {
                ressource_data[j] = (domain_load_t) {0, 0.0, 0.0, 0};
            }

            max_domain_id = domid_list[i];
        }

        // save current domain load
        ressource_data[domid_list[i]].iterations++;
        ressource_data[domid_list[i]].dom_id = domid_list[i];
        ressource_data[domid_list[i]].cpu_load = cpuload;
        ressource_data[domid_list[i]].mem_load = memload;
    }

    return 0;
}

domain_load_t* RM_RESSOURCE_MODEL_get_ressource_data(int* num_entries)
{
    // TODO get all domains which need hardware adaption
    // TODO split to memory and cpu?
    domain_load_t* ret;
    ret = ressource_data;
    *num_entries = max_domain_id + 1;
    return ret;
    
}

