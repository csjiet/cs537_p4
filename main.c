#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"

int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 10000);
    printf("init %d\n", rc);

    // Testing MFS_Lookup
    int pinum = 1; // random parent inode number
    char *name;
    name = (char*) malloc(sizeof(char) * 28);
    rc = MFS_Lookup(pinum, name); // Should expect rc to be > 1 and name to hold some data
    printf("lookup %d\n", rc);
    
    rc = MFS_Shutdown();
    printf("shut %d\n", rc);
    return 0;
}
