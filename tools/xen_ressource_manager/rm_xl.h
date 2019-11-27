#ifndef RM_XL_H
#define RM_XL_H

int RM_XL_init(void);

void RM_XL_close(void);

/*
 * Do not forget to free returned int array!
 */
int* RM_XL_get_domain_list(int* num_dom_out);

int RM_XL_add_vcpu(int domid);

#endif

