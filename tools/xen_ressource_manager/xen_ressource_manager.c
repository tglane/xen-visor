#include <stdlib.h>
#include <stdio.h>

//#include <_paths.h>
#include <xenstore.h>

int main(void)
{
    struct xs_handle* xsh;
    //struct expanding_buffer ebuf;
    char* read_val;
    unsigned int read_length;

    int mem_load;
    double cpu_load;

    xsh = (struct xs_handle*) xs_open(0);
    if(xsh == NULL)
    {
        printf("Could not open xs_handle\n");
        return -1;
    }
    printf("xs_handle opened\n");

    read_val = xs_read(xsh, XBT_NULL, "/local/domain/0/data/memload", &read_length);
    if(read_val == NULL)
    {
        printf("Could not read path\n");
        return -1;
    }
    mem_load = atoi(read_val);

    read_val = xs_read(xsh, XBT_NULL, "/local/domain/0/data/cpuload", &read_length);
    if(read_val == NULL)
    {
        printf("Could not read path\n");
        return -1;
    }
    cpu_load = atof(read_val);

    printf("Memload: %d, CPU load: %f\n", mem_load, cpu_load);

    free(read_val);
    xs_close(xsh);
}
