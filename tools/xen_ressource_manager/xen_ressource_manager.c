#include <stdlib.h>
#include <stdio.h>

#include <rm_xenstore.h>
#include <rm_xl.h>

int main(void)
{
    int memload, num_domains, i;
    double cpuload;
    int* domid_list;

    RM_XL_init();
    domid_list = RM_XL_get_domain_list(&num_domains);

    if(RM_XENSTORE_init() < 0)
        return -1;

    for(i = 0; i < num_domains; i++)
    {
        memload = RM_XENSTORE_read_domain_memload(domid_list[i]);
        cpuload = RM_XENSTORE_read_domain_cpuload(domid_list[i]);

        printf("domain_id: %d, memload: %d, cpuload: %f\n", domid_list[i], memload, cpuload);
    }
    RM_XENSTORE_close();

    if(RM_XL_add_vcpu(1) == 0)
        printf("added vcpu to domain 1");

    RM_XL_close();
    free(domid_list);

}
