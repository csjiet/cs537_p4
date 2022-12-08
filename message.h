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


typedef struct {
    // Fields to be updated BEFORE sending to the SERVER
    int c_sent_mtype; // message type from above
    int c_sent_inum; // inode number

    // Fields to be updated BEFORE sending back to the CLIENT
    int c_received_rc; // return code
    char* c_received_data; // real data chunk from malloc
    // put more here ...

} message_t;

#endif // __message_h__
