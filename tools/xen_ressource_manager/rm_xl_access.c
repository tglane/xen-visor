#include <rm_xl_access.h>

#include <stdio.h>

#include <stdlib.h>
#include <libxl.h>
#include <libxl_utils.h>
#include <libxlutil.h>
#include <xentoollog.h>

static libxl_ctx* ctx;
xentoollog_logger_stdiostream* logger;

int RM_XL_ACCESS_init(void)
{
    logger = xtl_createlogger_stdiostream(stderr, XTL_PROGRESS, 0);
    if(logger == NULL)
        return -1;

    libxl_ctx_alloc(&ctx, LIBXL_VERSION, 0, (xentoollog_logger*) logger);
    if(ctx == NULL)
        return -1;
    
    return 0;
}

void RM_XL_ACCESS_close(void)
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

int RM_XL_ACCESS_test(void)
{
    struct libxl_dominfo* info;
    int info_length, i;

    //libxl_dominfo_init(info);
    info = libxl_list_domain(ctx, &info_length);
    if(info == NULL)
        return -1;

    for(i = 0; i < info_length; i++)
    {
        char* domname;
        domname = libxl_domid_to_name(ctx, info[i].domid);
        printf("id: %5d, name: %-40s", info[i].domid, domname);
        free(domname);
    }
    
    libxl_dominfo_list_free(info, info_length);
    
    return 0;
}

