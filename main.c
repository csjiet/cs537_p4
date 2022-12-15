#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"
#include "ufs.h"
#include <string.h>

int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 10000);
    printf("Init: %d\n", rc);
 
    /**
     * Test 1: Tests a single create at the root directory
    */
    // rc = MFS_Creat(0, MFS_DIRECTORY, "test");
    // printf("Creat()'s return code: %d\n", rc);
  
    /**
     * Test 2: Tests if a single create in a FULL root directory prompts the creation of new block
    */
   char des[] = "test";
   char buf[6];
    for(int i = 0; i< 128 - 1; i++){
        sprintf(buf, "%d", i);
        strcat(des, buf);
        rc = MFS_Creat(0, MFS_DIRECTORY, des);
        printf("Return code MFS_Creat(): %d\n", rc);
    }

    rc = MFS_Shutdown();
    printf("shut %d\n", rc);
    return 0;
}
 