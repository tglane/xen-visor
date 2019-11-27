#ifndef RM_XENSTORE_ACCESS_H
#define RM_XENSTORE_ACCESS_H

#include <xenstore.h>

int RM_XENSTORE_ACCESS_init(void);

void RM_XENSTORE_ACCESS_close(void);

int RM_XENSTORE_ACCESS_read_domain_memload(int domid);

double RM_XENSTORE_ACCESS_read_domain_cpuload(int domid);

#endif

