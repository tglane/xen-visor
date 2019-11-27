#include <stdlib.h>
#include <stdio.h>

#include <rm_xenstore_access.h>
#include <rm_xl_access.h>

int main(void)
{
    int memload, num_domains, i;
    double cpuload;
    int* domid_list;

    RM_XL_ACCESS_init();
    domid_list = RM_XL_ACCESS_get_domain_list(&num_domains);

    if(RM_XENSTORE_ACCESS_init() < 0)
        return -1;

    for(i = 0; i < num_domains; i++)
    {
        memload = RM_XENSTORE_ACCESS_read_domain_memload(domid_list[i]);
        cpuload = RM_XENSTORE_ACCESS_read_domain_cpuload(domid_list[i]);

        printf("domain_id: %d, memload: %d, cpuload: %f\n", domid_list[i], memload, cpuload);
    }
    RM_XENSTORE_ACCESS_close();

    RM_XL_ACCESS_close();
    free(domid_list);

}
