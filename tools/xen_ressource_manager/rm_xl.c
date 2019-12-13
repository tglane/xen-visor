#include <rm_xl.h>

#include <stdlib.h>
#include <libxl_utils.h>
#include <libxlutil.h>
#include <xentoollog.h>

#define DOMAIN_MIN_MEM 250000

static libxl_ctx* ctx;
static xentoollog_logger_stdiostream* logger;

int RM_XL_init(void)
{
    logger = xtl_createlogger_stdiostream(stderr, XTL_PROGRESS, 0);
    if(logger == NULL)
        return -1;

    libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, (xentoollog_logger*) logger);
    if(ctx == NULL)
    {
        RM_XL_close();
        return -1;
    }
    return 0;
}

void RM_XL_close(void)
{
    if(ctx)
    {
        libxl_ctx_free(ctx);
        ctx = NULL;
    }
    if(logger)
    {
        xtl_logger_destroy((xentoollog_logger*) logger);
        logger = NULL;
    }
}

libxl_dominfo* RM_XL_get_domain_list(int* num_dom_out)
{
    struct libxl_dominfo* info;

    if(ctx == NULL)
        return NULL;

    info = libxl_list_domain(ctx, num_dom_out);
    if(info == NULL)
        return NULL;
    
    return info;
}

int RM_XL_get_host_cpu(void)
{
    if(ctx == NULL)
        return -1;

    return libxl_get_max_cpus(ctx);
}

int64_t RM_XL_get_host_mem_total(void)
{
    libxl_physinfo info;
    const libxl_version_info* vinfo;
    unsigned int i;

    if(ctx == NULL)
        return -1;    

    if(libxl_get_physinfo(ctx, &info) != 0)
        return -1;
    
    vinfo = libxl_get_version_info(ctx);
    i = (1 << 20) / vinfo->pagesize;
    
    return (info.total_pages / i) * 1000;
}

int RM_XL_change_vcpu(int domid, int change_vcpus)
{
    libxl_dominfo domain_info;
    libxl_bitmap cpu_map;
    unsigned int i, online_vcpus, host_cpus;

    if(ctx == NULL || change_vcpus == 0)
        return -1;

    host_cpus = libxl_get_max_cpus(ctx);
    libxl_domain_info(ctx, &domain_info, domid);
    online_vcpus = domain_info.vcpu_online;

    if(online_vcpus + change_vcpus > host_cpus || online_vcpus + change_vcpus <= 0)
    {
        libxl_dominfo_dispose(&domain_info);
        return -1;
    }

    libxl_cpu_bitmap_alloc(ctx, &cpu_map, online_vcpus + 1);
    for(i = 0; i < online_vcpus + change_vcpus; i++)
    {
        libxl_bitmap_set(&cpu_map, i);
    }
    libxl_set_vcpuonline(ctx, domid, &cpu_map, NULL);

    libxl_bitmap_dispose(&cpu_map);
    libxl_dominfo_dispose(&domain_info);
    return 0;
}

int RM_XL_change_memory(int domid, int64_t change_kb)
{
    // TODO what about domains static-max memory value?

    libxl_physinfo info;
    uint64_t memory_online;

    if(ctx == NULL || change_kb == 0)
        return -1;

    if(libxl_get_memory_target(ctx, domid, &memory_online))
        return -1;

    if(libxl_get_physinfo(ctx, &info) != 0)
        return -1;
     
    memory_online += change_kb;
    if(memory_online >= DOMAIN_MIN_MEM && memory_online <= RM_XL_get_host_mem_total())
    {
        if(libxl_set_memory_target(ctx, domid, memory_online, 0, 1))
            return -1;
        return 0;
    }
    return -1;
}

