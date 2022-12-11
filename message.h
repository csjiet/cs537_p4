#ifndef __message_h__
#define __message_h__

#define MFS_INIT (1)
#define MFS_LOOKUP (2)
#define MFS_STAT (3)
#define MFS_WRITE (4)
#define MFS_READ (5)
#define MFS_CRET (6)
#define MFS_UNLINK (7)
#define MFS_SHUTDOWN (8)
#include "mfs.h"

typedef struct {
    // Fields to be updated BEFORE sending to the SERVER
    int c_sent_mtype; // ALWAYS ASSIGNED BY CLIENT
    int c_sent_inum;
    char c_sent_name[100]; 
    char c_sent_buffer[4096];
    int c_sent_offset;
    int c_sent_nbytes;
    int c_sent_ftype;

    // Fields to be updated BEFORE sending back to the CLIENT
    int c_received_rc; // ALWAYS ASSIGNED BY SERVER
    int c_received_inum; 
    char c_received_buffer[4096];
    int c_received_buffer_size;
    MFS_Stat_t c_received_mfs_stat;
    MFS_DirEnt_t c_received_mfs_dirent;
    


    // put more here ...

    /*
    
    int MFS_Init(char *hostname, int port);
    int MFS_Lookup(int pinum, char *name); - sends: int(inum), char[](name); gets: inum of the file
    int MFS_Stat(int inum, MFS_Stat_t *m); - sends: int(inum); gets: returns MFS_Stat_t
    int MFS_Write(int inum, char *buffer, int offset, int nbytes); - sends: int(inum), char*(buffer), int(offset), int(nbytes); returns: c_received_rc; 
    int MFS_Read(int inum, char *buffer, int offset, int nbytes); - sends: int(inum), char*(buffer), int(offset), int(nbytes); returns: MFS_DirEnt_t
    int MFS_Creat(int pinum, int type, char *name); - sends: int(pinum), int(type), char*(name); returns: c_received_rc;
    int MFS_Unlink(int pinum, char *name); - sends: int(pinum), char*(name); returns: c_received_rc;
    int MFS_Shutdown(); - sends: nothing; returns: c_received_rc;

    */

} message_t;

#endif // __message_h__
