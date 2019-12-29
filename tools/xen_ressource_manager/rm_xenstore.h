#ifndef RM_XENSTORE_H
#define RM_XENSTORE_H

/**
 * RM_XENSTORE is used to access functions from libxenstore in a save and simple way
 * Used to get usage data of the running domains from the XenStore
 *
 * Usage Data is stored under /local/domain/x/data/y
 * where x is the domain id and y is {cpuload | memload | memtotal}
 */

/**
 * Initialize the context handle for accessing the XenStore
 * Accessing XenStore without initialization won't work
 *
 * Returns 0 if initialization was done or -1 if not
 */
int RM_XENSTORE_init(void);

/**
 * Free all XenStore-related data structures
 */
void RM_XENSTORE_close(void);

/**
 * Check if all handles have been initialized
 *
 * Returns 1 if everything is initialized and 0 if not 
 */
int RM_XENSTORE_initialized(void);

/**
 * Reads total memory and current memory usage of a domain from XenStore and calculates usage
 * 
 * Returns memory usage in percent (0.00 - 100.00)
 * Parameter domid is the id of the domain
 */
double RM_XENSTORE_read_domain_memload(int domid);

/**
 * Reads the cpu usage in percent of a domain from the XenStore
 *
 * Returns cpu usage in percent (0.00 - 100.00)
 * Parameter domid is the id of the domain
 */
double RM_XENSTORE_read_domain_cpuload(int domid);

#endif

