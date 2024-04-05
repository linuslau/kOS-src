void*   memcpy(void* p_dst, void* p_src, int size);
void    memset(void* p_dst, char ch, int size);
int     strlen(const char* p_str);
int     memcmp(const void * s1, const void *s2, int n);
int     strcmp(const char * s1, const char *s2);
char*   strcat(char * s1, const char *s2);

/* phys_copy/phys_set for kernel, segments are flat (based on 0), linear address is mapped to the identical physical address.
 * so they are mapped to memcpy/memset. */
#define	phys_copy	memcpy
#define	phys_set	memset

