#include <stdlib.h>
#include <stdio.h>

#include <rm_xenstore_access.h>
#include <rm_xl_access.h>

//#include <_paths.h>

int main(void)
{
    int memload;
    double cpuload;

    if(RM_XENSTORE_ACCESS_init() < 0)
        return -1;

    memload = RM_XENSTORE_ACCESS_read_domain_memload(0);
    cpuload = RM_XENSTORE_ACCESS_read_domain_cpuload(0);

    printf("Mem: %d, CPU: %f\n", memload, cpuload);
    
    RM_XENSTORE_ACCESS_close();

    RM_XL_ACCESS_init();
    RM_XL_ACCESS_close();

}
