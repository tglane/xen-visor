/**
 *  Resource management software used to dynamically adjust resource distribution to the active xen domains
 *  UNDER DEVELOPMENT
 *
 *  Author: Timo Glane <timo.glane@gmail.com>
 *
 */

#ifndef RESOURCE_MANAGEMENT_H
#define RESOURCE_MANAGEMENT_H

#include <xen/lib.h>
#include <xen/time.h>

/* Defines */
#define NO_DATA 999
#define IDLE_DOMAIN_ID 32767

struct domain_load {
    uint8_t domain_id;
    s_time_t last_timestamp;
    
    s_time_t last_cpu_time;

    uint8_t cpu_load;
};
typedef struct domain_load domain_load_t;

struct resource_model {
    size_t size;
    struct domain_load* load_data;
};
typedef struct resource_model resource_model_t;


struct domain_load* get_resource_model(void);

void free_resource_model(struct resource_model);

void update_resource_model(struct resource_model*);

#endif

