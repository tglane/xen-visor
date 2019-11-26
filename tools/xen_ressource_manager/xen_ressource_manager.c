#include <stdlib.h>
#include <stdio.h>

//#include <_paths.h>
#include <xenstore.h>

int main(void)
{
    struct xs_handle* xsh;
    char* mem_load;
    unsigned int read_length;
    xsh = (struct xs_handle*) xs_open(0);
    if(xsh == NULL)
    {
        printf("Could not open xs_handle\n");
        return -1;
    }

    mem_load = (char*) xs_read(xsh, XBT_NULL, "/local/domain/0/data/memload", &read_length);

    printf("Memload: %d", *mem_load);

    free(mem_load);
    xs_close(xsh);
}
