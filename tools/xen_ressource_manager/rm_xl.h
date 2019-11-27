#ifndef RM_XL_H
#define RM_XL_H

#include <stdlib.h>
#include <stdint.h>

int RM_XL_init(void);

void RM_XL_close(void);

/*
 * Do not forget to free returned int array!
 */
int* RM_XL_get_domain_list(int* num_dom_out);

int RM_XL_add_vcpu(int domid);

int RM_XL_add_memory(int domid, uint64_t add);

#endif

