#include <stdio.h>
#include "mfs.h"
#include "udp.h"


struct sockaddr_in addrSnd, addrRcv;
int sd, rc;

int MFS_Init(char *hostname, int port) {
    // do some net setup
    printf("MFS Init2 %s %d\n", hostname, port);
    sd = UDP_Open(20000);
    rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    return rc;
}

int MFS_Lookup(int pinum, char *name) {
    // network communication to do the lookup to server
    return 0;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
    return 0;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes) {
    return 0;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes) {
    return 0;
}

int MFS_Creat(int pinum, int type, char *name) {
    return 0;
}

int MFS_Unlink(int pinum, char *name) {
    return 0;
}

int MFS_Shutdown() {
    printf("MFS Shutdown\n");
    return 0;
}
