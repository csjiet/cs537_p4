#include <stdio.h>
#include "mfs.h"
#include "udp.h"
#include "message.h"
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct sockaddr_in addrSnd, addrRcv; // Create client socket for sending, and receiving
int sd, rc;

/*This function takes a host name and port number and uses those to find the server exporting the 
file system.*/ 
int MFS_Init(char *hostname, int port) {

    // Param checks: Failure modes: invalid inum, invalid offset, invalid nbytes.
    if(hostname == NULL)
        return -1;

    if(port < 0)
        return -1;

    //printf("MFS Init2 %s %d\n", hostname, port);
    sd = UDP_Open(20000); // Opens client socket with port 20000
    rc = UDP_FillSockAddr(&addrSnd, hostname, port); // Connects client socket to server socket
    return rc;
}

/*This function takes the parent inode number (which should be the inode number of a directory) and 
looks up the entry name in it. The inode number of name is returned. Success: return inode number of 
name; failure: return -1. Failure modes: invalid pinum, name does not exist in pinum.*/ 
int MFS_Lookup(int pinum, char *name) {

    // Param checks: Failure modes: invalid inum, invalid offset, invalid nbytes.
    if(pinum < 0)
        return -1;

    if(name == NULL)
        return -1;
    
    // Create message struct to be sent and received
    message_t msg;
    msg.c_sent_mtype = MFS_LOOKUP;
    msg.c_sent_inum = pinum;
    strcpy(msg.c_sent_name, name);

    // Write a request to server to retrieve data
    rc = UDP_Write(sd, &addrSnd, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Lookup WRITE failed; libmfs.c\n");
        return -1;
    }

    // Reads data sent back from server in message_t and returns a return code to check if server 
    // processsing was successful
    rc = UDP_Read(sd, &addrRcv, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Lookup READ failed; libmfs.c\n");
        return -1;
    }

    // For testing purposes
    //strcpy(name, m.c_received_data);
    //printf("client: Received data from server: %s\n", name);

    return msg.c_received_inum;
    
}

/*This function returns some information about the file specified by inum. Upon success, return 0, 
otherwise -1. The exact info returned is defined by MFS_Stat_t. Failure modes: inum does not exist. 
File and directory sizes are described below.

Information format:


*/
int MFS_Stat(int inum, MFS_Stat_t *m) {

    // Param checks: Failure modes: invalid inum, invalid offset, invalid nbytes.
    if(inum < 0)
        return -1;
    
    if(m == NULL)
        return -1;

    message_t msg;
    msg.c_sent_inum = inum;
    msg.c_sent_mtype = MFS_STAT;
    // Write a request to server to retrieve data
    rc = UDP_Write(sd, &addrSnd, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat WRITE failed; libmfs.c\n");
        return -1;
    }

    // Reads data sent back from server in message_t and returns a return code to check if server 
    // processsing was successful
    rc = UDP_Read(sd, &addrRcv, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat READ failed; libmfs.c\n");
        return -1;
    }

    m->type = msg.c_received_mfs_stat.type;
    m->size = msg.c_received_mfs_stat.size;

    return 0;
}

/* This function writes a buffer of size nbytes (max size: 4096 bytes) at the byte offset specified by offset. Returns 
0 on success, -1 on failure. Failure modes: invalid inum, invalid nbytes, invalid offset, not a regular 
file (because you can't write to directories).

Information format:
*/

int MFS_Write(int inum, char *buffer, int offset, int nbytes) {

    // Param checks: Failure modes: invalid inum, invalid offset, invalid nbytes.
    if(inum < 0)
        return -1;

    if(buffer == NULL)
        return -1;
    
    if(offset < 0)
        return -1;

    if(nbytes > 4096)
        return -1;
    
    if(nbytes < 0)
        return -1;

    message_t msg;
    msg.c_sent_inum = inum;
    strcpy(msg.c_sent_buffer, buffer);
    msg.c_sent_offset = offset;
    msg.c_sent_nbytes = nbytes;
    msg.c_sent_mtype = MFS_WRITE;

    // Write a request to server to retrieve data
    rc = UDP_Write(sd, &addrSnd, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat WRITE failed; libmfs.c\n");
        return -1;
    }

    // Reads data sent back from server in message_t and returns a return code to check if server 
    // processsing was successful
    rc = UDP_Read(sd, &addrRcv, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat READ failed; libmfs.c\n");
        return -1;
    }
    return 0;
}

/*This function reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the 
buffer from file specified by inum. The routine should work for either a file or directory; directories 
should return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1. Failure modes: invalid inum, 
invalid offset, invalid nbytes.

Information format:
*/
int MFS_Read(int inum, char *buffer, int offset, int nbytes) {

    // Param checks: Failure modes: invalid inum, invalid offset, invalid nbytes.
    if(inum < 0)
        return -1;
    
    if(buffer == NULL)
        return -1;

    if(offset < 0)
        return -1;
    
    if(nbytes > 4096)
        return -1;
    
    if (nbytes < 0)
        return -1;


    // LOOKUP SHOULD BE CALLED SOMEWHERE HERE!

    message_t msg;
    msg.c_sent_inum = inum;
    strcpy(msg.c_sent_buffer, buffer);
    msg.c_sent_offset = offset;
    msg.c_sent_nbytes = nbytes;
    msg.c_sent_mtype = MFS_READ;

    // Write a request to server to retrieve data
    rc = UDP_Write(sd, &addrSnd, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat WRITE failed; libmfs.c\n");
        return -1;
    }

    // Reads data sent back from server in message_t and returns a return code to check if server 
    // processsing was successful
    rc = UDP_Read(sd, &addrRcv, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat READ failed; libmfs.c\n");
        return -1;
    }
    //msg.c_received_mfs_dirent;

    return 0;
}

/* This function makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified 
by pinum of name name. Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long. 
If name already exists, return success.

Information format:
*/
int MFS_Creat(int pinum, int type, char *name) {
    // Param checks: Failure modes: invalid inum, invalid offset, invalid nbytes.
    
    if(pinum < 0)
        return -1;

    if(name == NULL)
        return -1;

    if(strlen(name) > 28) {
        return -1;
    } 

    message_t msg;
    msg.c_sent_inum = pinum;
    msg.c_sent_ftype = type;
    strcpy(msg.c_sent_name, name);
    msg.c_sent_mtype = MFS_CRET;
    
    // Write a request to server to retrieve data
    rc = UDP_Write(sd, &addrSnd, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat WRITE failed; libmfs.c\n");
        return -1;
    }

    // Reads data sent back from server in message_t and returns a return code to check if server 
    // processsing was successful
    rc = UDP_Read(sd, &addrRcv, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat READ failed; libmfs.c\n");
        return -1;
    }
    
    return msg.c_received_rc;
}

/* This function removes the file or directory name from the directory specified by pinum. 0 on success, -1 on failure. Failure modes: 
pinum does not exist, directory is NOT empty. Note that the name not existing is NOT a failure by our definition (think 
about why this might be).

Information format:
*/
int MFS_Unlink(int pinum, char *name) {
    if(pinum < 0)
        return -1;

    // NAME BEING NULL ISN'T BAD???
    if (name == NULL)
        return -1;
    message_t msg;
    msg.c_sent_inum = pinum;
    strcpy(msg.c_sent_name, name);
    msg.c_sent_mtype = MFS_UNLINK;

    // Write a request to server to retrieve data
    rc = UDP_Write(sd, &addrSnd, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat WRITE failed; libmfs.c\n");
        return -1;
    }

    // Reads data sent back from server in message_t and returns a return code to check if server 
    // processsing was successful
    rc = UDP_Read(sd, &addrRcv, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Stat READ failed; libmfs.c\n");
        return -1;
    }
    return msg.c_received_rc;
}

/*This function  just tells the server to force all of its data structures to disk and shutdown by calling exit(0). This interface 
will mostly be used for testing purposes.

Information format:
*/
int MFS_Shutdown() {
    //printf("MFS Shutdown\n");
  
    message_t msg;
    msg.c_sent_mtype = MFS_SHUTDOWN;

    rc = UDP_Write(sd, &addrSnd, (char*) &msg, sizeof(message_t));
    if(rc < 0){
        printf("client:: MFS_Shutdown failed; libmfs.c\n");
        return -1;
    }    
   
    // UDP_Close(20000);
    UDP_Close(sd);
    return 0;
}
