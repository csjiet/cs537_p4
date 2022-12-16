#include <stdio.h>
#include <stdlib.h>
#include "mfs.h"
#include "ufs.h"
#include <string.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 10100);
    printf("Init: %d\n", rc);
 
    /**
     * Test 1: Tests a single create at the root directory
    */
    // rc = MFS_Creat(0, MFS_DIRECTORY, "test");
    // printf("Creat()'s return code: %d\n", rc);
  
    /**
     * Test 2: Tests if a single create in a FULL root directory prompts the creation of new block
    */
//    char des[] = "test";
//    char buf[6];
//    // 128 -1 so that is just exceeds the first block slightly
//     for(int i = 0; i< 128 - 1; i++){
//         sprintf(buf, "%d", i);
//         strcat(des, buf);
//         rc = MFS_Creat(0, MFS_DIRECTORY, des);
//         printf("Return code MFS_Creat(): %d\n", rc);
//     }

    /**
     * Test 3: Tests if a single create in a FULL root directory prompts the creation of new block
    */
//    char des[] = "test";
//    char buf[6];
//    // 128 -1 so that is just exceeds the first block slightly
//     for(int i = 0; i< 128 - 1; i++){
//         sprintf(buf, "%d", i);
//         strcat(des, buf);
//         rc = MFS_Creat(0, MFS_REGULAR_FILE, des);
//         printf("Return code MFS_Creat(): %d\n", rc);
//     }

    /**
     * Tests 4: Test lookup
    */
   rc = MFS_Creat(0, MFS_DIRECTORY, "testdir");
   rc = MFS_Lookup(0, "testdir");
   printf("SEARCHING FOR . !!!!!!!!!!!!!!!\n");
   rc = MFS_Lookup(1, ".");
   printf("SEARCHING FOR .. !!!!!!!!!!!!!!!\n");
   rc = MFS_Lookup(1, "..");
   assert(rc > -1);


    rc = MFS_Shutdown();
    printf("shut %d\n", rc);
    return 0;
}
 