#ifndef RM_XENSTORE_H
#define RM_XENSTORE_H

int RM_XENSTORE_init(void);

void RM_XENSTORE_close(void);

double RM_XENSTORE_read_domain_memload(int domid);

double RM_XENSTORE_read_domain_cpuload(int domid);

#endif

