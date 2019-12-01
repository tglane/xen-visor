#ifndef RM_RESSOURCE_MODEL_H
#define RM_RESSOURCE_MODEL_H

struct domain_load {
    int dom_id;
    double cpu_load;
    double mem_load;
    long iterations;
};
typedef struct domain_load domain_load_t;

void RM_RESSOURCE_MODEL_init(void);

void RM_RESSOURCE_MODEL_free(void);

int RM_RESSOURCE_MODEL_update(int* domid_list, int num_domains);

domain_load_t* RM_RESSOURCE_MODEL_get_ressource_data(int* num_entries);

#endif

