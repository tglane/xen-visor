#ifndef RM_XL_ACCESS_H
#define RM_XL_ACCESS_H

int RM_XL_ACCESS_init(void);

void RM_XL_ACCESS_close(void);

/*
 * Do not forget to free returned int array!
 */
int* RM_XL_ACCESS_get_domain_list(int* num_dom_out);

#endif

