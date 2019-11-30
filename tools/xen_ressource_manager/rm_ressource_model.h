#ifndef RM_RESSOURCE_MODEL_H
#define RM_RESSOURCE_MODEL_H

struct adaption_data {
    int* cpu_adaption_domains;
    int num_cpu_adaption_domains;
    int* mem_adaption_domains;
    int num_mem_adaption_domains;
};
typedef struct adaption_data adaption_data_t;

struct domain_load {
    int dom_id;
    double cpu_load;
    double mem_load;
};
typedef struct domain_load domain_load_t;

int RM_RESSOURCE_MODEL_update(int* domid_list, int num_domains);

domain_load_t* RM_RESSOURCE_MODEL_get_adaption_domains(int* num_entries);

#endif

