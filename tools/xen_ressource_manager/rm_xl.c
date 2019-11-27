#include <rm_xl.h>

#include <stdio.h>

#include <stdlib.h>
#include <libxl.h>
#include <libxl_utils.h>
#include <libxlutil.h>
#include <xentoollog.h>

static libxl_ctx* ctx;
static xentoollog_logger_stdiostream* logger;

int RM_XL_init(void)
{
    logger = xtl_createlogger_stdiostream(stderr, XTL_PROGRESS, 0);
    if(logger == NULL)
        return -1;

    libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, (xentoollog_logger*) logger);
    if(ctx == NULL)
        return -1;
    
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

int* RM_XL_get_domain_list(int* num_dom_out)
{
    struct libxl_dominfo* info;
    int i;
    int* domid_list;

    //libxl_dominfo_init(info);
    info = libxl_list_domain(ctx, num_dom_out);
    if(info == NULL)
        exit(-1);
    
    domid_list = malloc(*num_dom_out * sizeof(int));

    for(i = 0; i < *num_dom_out; i++)
    {
        //char* domname;
        domid_list[i] = info[i].domid;

        //domname = libxl_domid_to_name(ctx, info[i].domid);
        //printf("id: %5d, name: %-40s\n", info[i].domid, domname);
        //free(domname);
    }
    
    libxl_dominfo_list_free(info, *num_dom_out);
    
    return domid_list;
}

int RM_XL_add_vcpu(int domid)
{
    return 1;
}

