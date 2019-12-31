#include <rm_xenstore.h>

#include <stdlib.h>
#include <stdio.h>
#include <xenstore.h>

static struct xs_handle* xsh;

int RM_XENSTORE_init(void)
{
    xsh = (struct xs_handle*) xs_open(0);
    if(xsh == NULL)
        return -1;

    return 0;
}

void RM_XENSTORE_close(void)
{
    if(xsh != NULL)
        xs_close(xsh);
}

int RM_XENSTORE_initialized(void)
{
    if(xsh == NULL)
        return 0;
    else
        return 1;
}

double RM_XENSTORE_read_domain_memload(int domid)
{
    char path[32];
    char* read_val;
    unsigned int read_length;
    int mem_total, mem_usage;
    double mem_load;

    if(xsh == NULL)
        return -1;
    
    snprintf(path, 32, "/local/domain/%d/data/memload", domid);
    read_val = xs_read(xsh, XBT_NULL, path, &read_length);
    if(read_val == NULL)
        return -1;
    mem_usage = atoi(read_val);
    
    snprintf(path, 32, "/local/domain/%d/data/memtotal", domid);
    read_val = xs_read(xsh, XBT_NULL, path, &read_length);
    if(read_val == NULL)
        return -1;
    mem_total = atoi(read_val);
    
    mem_load = ((double) mem_usage / (double) mem_total) * 100;
    free(read_val);
    return mem_load;
}

double RM_XENSTORE_read_domain_cpuload(int domid)
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
    free(read_val);
    return cpuload;
}

int RM_XENSTORE_read_domain_priority(int domid)
{
    char path[31];
    char* read_val;
    unsigned int read_length; 
    int priority;

    if(xsh == NULL)
        return -1;

    snprintf(path, 31, "/local/domain/%d/data/priority", domid);
    read_val = xs_read(xsh, XBT_NULL, path, &read_length);
    if(read_val == NULL)
        return 3;

    priority = atoi(read_val);
    free(read_val);
    if(0 < priority && priority < 6)
        return priority;
    else
        return -1;
}

