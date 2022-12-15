#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"
#include "ufs.h"

int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 10000);
    printf("Init: %d\n", rc);
    // Testing MFS_Lookup
    int pinum = 1; // random parent inode number
    char *name;
    MFS_Stat_t* m = malloc(sizeof(MFS_Stat_t));
    //inode_t* inodet = malloc(sizeof(inode_t));
    m->size = 0;
    m->type = 0;
    name = (char*) malloc(sizeof(char) * 28);
    rc = MFS_Lookup(pinum, name); // Should expect rc to be > 1 and name to hold some data
    printf("lookup %d\n", rc);


    rc = MFS_Stat(0, m);
    printf("SIZE: %d, TYPE: %d\n", m->size, m->type);
    // rc = MFS_Creat(0, MFS_REGULAR_FILE, "test");
    // printf("Create: %d\n", rc);
    rc = MFS_Shutdown();
    printf("shut %d\n", rc);
    return 0;
}
