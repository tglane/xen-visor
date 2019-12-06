#include <rm_ressource_model.h>

#include <stdlib.h>
#include <rm_xenstore.h>

#define WEIGHT 0.75

static domain_load_t* ressource_data;
static int max_domain_id = 0;

void RM_RESSOURCE_MODEL_init(void)
{
    if(ressource_data == NULL)
    {
        ressource_data = malloc(sizeof(domain_load_t));
        ressource_data[0] = (domain_load_t) {-1, 0.0, 0.0};
    }
}

void RM_RESSOURCE_MODEL_free(void)
{
    if(ressource_data != NULL)
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
        // Realloc memory for ressource_data if max_domain_id is lower than current id
        if(domid_list[i] > max_domain_id)
        {
            ressource_data = realloc(ressource_data, (domid_list[i] + 1) * sizeof(domain_load_t));
            if(ressource_data == NULL)
                return -1;

            for(j = max_domain_id + 1; j < domid_list[i]; j++)
            {
                ressource_data[j] = (domain_load_t) {-1, 0.0, 0.0};
            }

            max_domain_id = domid_list[i];
        }

        memload = RM_XENSTORE_read_domain_memload(domid_list[i]);
        cpuload = RM_XENSTORE_read_domain_cpuload(domid_list[i]);
      
        // Save current domain load and calculate average load only if there is load data
        if(memload < 0 || cpuload < 0)
        {
            ressource_data[domid_list[i]].dom_id = -1;
            ressource_data[domid_list[i]].cpu_load = 0;
            ressource_data[domid_list[i]].mem_load = 0;
        }
        else
        {
            // Calculate exponential moving average
            ressource_data[domid_list[i]].dom_id = domid_list[i];
            ressource_data[domid_list[i]].cpu_load = 
                (WEIGHT * cpuload) + (1.0 - WEIGHT) * ressource_data[domid_list[i]].cpu_load;
            ressource_data[domid_list[i]].mem_load = 
                (WEIGHT * memload) + (1.0 - WEIGHT) * ressource_data[domid_list[i]].mem_load;

            // Moving average
            /*ressource_data[domid_list[i]].iterations++;

            ressource_data[domid_list[i]].dom_id = domid_list[i];
            
            ressource_data[domid_list[i]].cpu_load = ressource_data[domid_list[i]].cpu_load + 
                ((cpuload - ressource_data[domid_list[i]].cpu_load) / ressource_data[domid_list[i]].iterations);
            
            ressource_data[domid_list[i]].mem_load = ressource_data[domid_list[i]].mem_load + 
                ((memload - ressource_data[domid_list[i]].mem_load) / ressource_data[domid_list[i]].iterations);*/
        }
    }

    return 0;
}

domain_load_t* RM_RESSOURCE_MODEL_get_ressource_data(int* num_entries)
{
    *num_entries = max_domain_id + 1;
    return ressource_data;
    
}

