#include <rm_xenstore_access.h>

#include <stdlib.h>
#include <stdio.h>

static struct xs_handle* xsh;

int RM_XENSTORE_ACCESS_init(void)
{
    xsh = (struct xs_handle*) xs_open(0);
    if(xsh == NULL)
        return -1;

    return 0;
}

void RM_XENSTORE_ACCESS_close(void)
{
    if(xsh != NULL)
        xs_close(xsh);
}

int RM_XENSTORE_ACCESS_read_domain_memload(int domid)
{
    char path[31];
    char* read_val;
    unsigned int read_length;
    int memload;

    if(xsh == NULL)
        return -1;
    
    snprintf(path, 31, "/local/domain/%d/data/memload", domid);
    read_val = xs_read(xsh, XBT_NULL, path, &read_length);
    if(read_val == NULL)
        return -1;

    memload = atoi(read_val);
    return memload;
}

double RM_XENSTORE_ACCESS_read_domain_cpuload(int domid)
{
    char path[31];
    char* read_val;
    unsigned int read_length;
    double cpuload;

    if(xsh == NULL)
        return -1;

    snprintf(path, 31, "/local/domain/%d/data/cpuload", domid);
    read_val = xs_read(xsh, XBT_NULL, path, &read_length);
    if(read_val == NULL)
        return -1;

    cpuload = atof(read_val);
    return cpuload;
}

