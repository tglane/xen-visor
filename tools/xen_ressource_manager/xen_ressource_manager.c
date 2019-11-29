#include <stdlib.h>
#include <stdio.h>

#include <rm_xenstore.h>
#include <rm_xl.h>

int main_ressource_checker(void)
{
    int num_domains, i;
    double memload, cpuload;
    int* domid_list;

    RM_XL_init();
    domid_list = RM_XL_get_domain_list(&num_domains);
    if(domid_list == NULL)
        return -1;

    if(RM_XENSTORE_init() < 0)
        return -1;

    for(i = 0; i < num_domains; i++)
    {
        memload = RM_XENSTORE_read_domain_memload(domid_list[i]);
        cpuload = RM_XENSTORE_read_domain_cpuload(domid_list[i]);

        printf("domain_id: %d, memload: %f, cpuload: %f\n", domid_list[i], memload, cpuload);
    }
    RM_XENSTORE_close();

    if(RM_XL_change_vcpu(2, -2) == 0)
        printf("added vcpu to domain 1\n");

    if(RM_XL_change_memory(2, -200000) == 0)
        printf("added memory to domain 1\n");

    RM_XL_close();
    free(domid_list);
}

void main(void)
{
    main_ressource_checker();
}

