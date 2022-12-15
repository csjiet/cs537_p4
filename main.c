#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"
#include "ufs.h"

int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 10000);
    printf("Init: %d\n", rc);
 

    rc = MFS_Creat(0, MFS_DIRECTORY, "test");
    printf("Creat()'s return code: %d\n", rc);
  
    rc = MFS_Shutdown();
    printf("shut %d\n", rc);
    return 0;
}
 