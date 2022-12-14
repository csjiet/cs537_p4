#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include "ufs.h"
#include "udp.h"
#include "message.h"


#define BLOCKSIZE (4096)

static int PORTNUM;
static char* IMGFILENAME;
bool isShutdown = false;

static super_t* SUPERBLOCKPTR;
int sd; // Server file descriptor
int fd; // mmap file descriptor
void *image;

// Struct definitions
typedef struct {
	inode_t inodes[UFS_BLOCK_SIZE / sizeof(inode_t)];
} inode_block_t;

typedef struct {
	dir_ent_t entries[128];
} dir_block_t;

typedef struct {
	unsigned int bits[UFS_BLOCK_SIZE / sizeof(unsigned int)];
} bitmap_t;


void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
}

/*This function returns some information about the file specified by inum. Upon success, return 0, 
otherwise -1. The exact info returned is defined by MFS_Stat_t. Failure modes: inum does not exist. 
File and directory sizes are described below.*/

int run_stat(message_t* m){
	//MFS_Stat_t stat;
	// Search for the file/ directory of the inode number
	int inum = m->c_sent_inum;

	// INODE BITMAP
	char bufBlock[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	printf("SUPERBLOCKPTR->inode_bitmap_addr: %d\n", SUPERBLOCKPTR->inode_bitmap_addr);
	printf("SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE: %d\n", SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE);

	unsigned int bitVal = get_bit((unsigned int*) bufBlock, inum);
	printf("bitval: %d\n", bitVal);
	if(bitVal == 0) {
		return -1;
	}
	// Find which block we are in before doing the below code.
	// 32 inums per block.

	// INODE TABLE
	int blockNumberOffsetInInodeTable = ceil((inum * sizeof(inode_t)) / BLOCKSIZE);
	printf("blockNumberOffset: %d\n", blockNumberOffsetInInodeTable);
	
	char bufBlock1[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock1, BLOCKSIZE);
	printf("Inoderegionaddr: %d\n", (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable));

	inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock1;
	// Same method for offset here


	int remainingInodeOffset = ceil((inum * sizeof(inode_t)) % BLOCKSIZE);
	//int remainingInodeOffset = inum - (blockNumberOffsetInInodeTable * 128);
	// ethan: CHANGE TO MODULUS	
	printf("Remaininginodeoffset: %d\n", remainingInodeOffset);
	
	inode_t inode = inodeBlockPtr->inodes[remainingInodeOffset/ sizeof(inode_t)];
	printf("inode[remainingInodeOffset/ sizeof(inode_t)]: %ld\n", remainingInodeOffset/ sizeof(inode_t));

	printf("inode type: %d\n", inode.type);
	printf("inode size: %d\n", inode.size);

	m->c_received_mfs_stat.size = inode.size;
	m->c_received_mfs_stat.type = inode.type;

	if (m->c_received_mfs_stat.type != 0) {
		if (m->c_received_mfs_stat.type != 1)
			return -1;
	}

	return 0;
}

/* This function writes a buffer of size nbytes (max size: 4096 bytes) at the byte offset specified by offset. Returns 
0 on success, -1 on failure. Failure modes: invalid inum, invalid nbytes, invalid offset, not a regular 
file (because you can't write to directories).   typedef struct {
	dir_ent_t entries[128];
    } dir_block_t;

Information format:
*/
int run_write(message_t* m){
	//ADD CHECK FOR REGULAR FILE TYPE. FOUND IN THE Structs
	int inum = m->c_sent_inum;
	int offset = m->c_sent_offset;
	int nbytes = m->c_sent_nbytes;
	// char* buffer = strdup(m->c_sent_buffer);
	// printf("%s\n", &buffer);
	if (inum < 0) 
		return -1;
	if (offset < 0)
		return -1;
	if (nbytes > 4096)
		return -1;

	// INODE BITMAP
	char bufBlock[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);
	unsigned int bitVal = get_bit((unsigned int*) bufBlock, inum);
	if(bitVal == 0)
		return -1;

	// INODE TABLE
	// Get Inode address in table
	int blockNumberOffsetInInodeTable = ceil((inum * sizeof(inode_t))/ BLOCKSIZE);
	char bufBlock2[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock2, BLOCKSIZE);

	inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock2;
	int remainingInodeOffset = (inum * sizeof(inode_t)) % BLOCKSIZE;

	inode_t inode = inodeBlockPtr->inodes[remainingInodeOffset/ sizeof(inode_t)];
	// IF inum type is Directory return -1. Can only write to regular files.
	if (inode.type == 0) {
		return -1;
	} else {
		// Find if we need 1 or two blocks to write. Minimum 1, max 2. 
		if ((SUPERBLOCKPTR->inode_region_addr + offset + nbytes) > 4096) {
			//Write to two blocks
			
		}
		else {
			//Write to a single block
			
		}
	}


	fsync(fd); // Idempotency from writeup
	return 0;
}

int offsetToFile(inode_t inode, int offset, int nbytes, message_t* m){

	int dirPtrBlockIndex = offset / BLOCKSIZE;
	int remainingOffsetWithinABlock = offset % BLOCKSIZE;

	int blockNumber = inode.direct[dirPtrBlockIndex];
	lseek(fd, (blockNumber * BLOCKSIZE) + remainingOffsetWithinABlock, SEEK_SET);

	char bufBlock[BLOCKSIZE];
	read(fd, bufBlock, BLOCKSIZE);

	// ETHAN: HANDLE THE CASE WHERE YOUR OFFSET IS CROSSING A BLOCK INTO ANOTHER BLOCK.
	// THEN YOUR BUFFER WHICH IS 4KB, STILL NEEDS TO SAVE PARTIAL DATA FROM 1 BLOCK, AND ANOTHER PARTIAL FROM ANOTHER BLOCK

	m->c_received_buffer_size = nbytes;
	strcpy(m->c_received_buffer, bufBlock);

	// Not sure if this is correct
	// MFS_DirEnt_t* dirEntryPtr = (MFS_DirEnt_t*) bufBlock;
	// m->c_received_mfs_dirent.inum = dirEntryPtr->inum;
	// strcpy(m->c_received_mfs_dirent.name, dirEntryPtr->name);
	
	return 0;
}

int offsetToDirectory(inode_t inode, int offset, int nbytes, message_t* m){

	int dirPtrBlockIndex = offset / BLOCKSIZE;
	int remainingOffsetWithinABlock = offset % BLOCKSIZE;

	int blockNumber = inode.direct[dirPtrBlockIndex];
	lseek(fd, (blockNumber * BLOCKSIZE) + remainingOffsetWithinABlock, SEEK_SET);

	char bufBlock[BLOCKSIZE];
	read(fd, bufBlock, BLOCKSIZE);

	// ETHAN: HANDLE THE CASE WHERE YOUR OFFSET IS CROSSING A BLOCK INTO ANOTHER BLOCK.
	// THEN YOUR BUFFER WHICH IS 4KB, STILL NEEDS TO SAVE PARTIAL DATA FROM 1 BLOCK, AND ANOTHER PARTIAL FROM ANOTHER BLOCK

	m->c_received_buffer_size = nbytes;
	strcpy(m->c_received_buffer, bufBlock);

	// Not sure if this is correct
	// MFS_DirEnt_t* dirEntryPtr = (MFS_DirEnt_t*) bufBlock;
	// m->c_received_mfs_dirent.inum = dirEntryPtr->inum;
	// strcpy(m->c_received_mfs_dirent.name, dirEntryPtr->name);


	return 0;
}

/*This function reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the 
buffer from file specified by inum. The routine should work for either a file or directory; directories 
should return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1. Failure modes: invalid inum, 
invalid offset, invalid nbytes.

Information format:
*/
int run_read(message_t* m){
	// Get the searched inode number
	int inum = m->c_sent_inum;
	int offset = m->c_sent_offset;
	int nbytes = m->c_sent_nbytes; // size: 1 - 4096
	
	// INODE BITMAP
	// Gets inode bitmap's location
	char bufBlock[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	// Read inode bitmap AND get bit of inode
	unsigned int bitVal = get_bit((unsigned int*) bufBlock, inum);

	// Check if inode is not found
	if(bitVal == 0)
		return -1;


	// INODE TABLE
	// Gets inode table's location
	int blockNumberOffsetInInodeTable = ceil((inum * sizeof(inode_t))/ BLOCKSIZE);
	char bufBlock1[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock1, BLOCKSIZE);

	inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock1;

	int remainingInodeOffset = inum - (blockNumberOffsetInInodeTable * 128); // CHANGE THIS TO MODULUS

	inode_t inode = inodeBlockPtr->inodes[remainingInodeOffset/ sizeof(inode_t)];



	// DATA BITMAP
	// Check whether the data region exists a data allocation for inode
	char bufBlock2[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->data_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock2, BLOCKSIZE);

	bitVal = get_bit((unsigned int*) bufBlock2, inum);

	// Check if inode data is allocated 
	if(bitVal == 0)
		return -1;

	// Check if file is regular file or directory
	int fileType = inode.type;

	// DATA REGION
	int rcDirPtr;
	if(fileType == MFS_DIRECTORY){
		rcDirPtr = offsetToDirectory(inode, offset, nbytes, m);
	}else{
		rcDirPtr = offsetToFile(inode, offset, nbytes, m);
	}
	
	if(rcDirPtr < 0)
		return -1;
	
	return 0;
}

/*This function takes the parent inode number (which should be the inode number of a directory) and 
looks up the entry name in it. The inode number of name is returned. Success: return inode number of 
name; failure: return -1. Failure modes: invalid pinum, name does not exist in pinum.*/ 
int run_lookup(message_t* m){
	// Get the searched inode number
	int pinum = m->c_sent_inum;
	char name[28];
	strcpy(name, m->c_sent_name);

	// INODE BITMAP
	char bufBlock[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);
	unsigned int bitVal = get_bit((unsigned int*) bufBlock, pinum);
	if(bitVal == 0)
		return -1;

	// INODE TABLE
	// Get Inode address in table
	int blockNumberOffsetInInodeTable = ceil((pinum * sizeof(inode_t))/ BLOCKSIZE);
	char bufBlock2[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock2, BLOCKSIZE);

	inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock2;
	int remainingInodeOffset = (pinum * sizeof(inode_t)) % BLOCKSIZE;
	//int remainingInodeOffset = pinum - (blockNumberOffsetInInodeTable * 128);
	//DO MODULUS

	inode_t pinode = inodeBlockPtr->inodes[remainingInodeOffset/ sizeof(inode_t)];

	
	// DATA BITMAP - check PARENT
	lseek(fd, SUPERBLOCKPTR->data_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);
	bitVal = get_bit((unsigned int*) bufBlock, pinum);
	if(bitVal == 0)
		return -1;

	// Check if inode is a parent
	if(pinode.type != MFS_DIRECTORY)
		return -1;

	// DATA REGION to parent directory
	// Iterate through all direct pointers in parent inode
	for(int i = 0; i< DIRECT_PTRS; i++){
		int blockNumber = pinode.direct[i];

		char bufBlockForDirBlock[BLOCKSIZE];
		lseek(fd, blockNumber * BLOCKSIZE, SEEK_SET);
		read(fd, bufBlock, BLOCKSIZE);
		dir_block_t* dirEntBlock = (dir_block_t*) bufBlockForDirBlock;

		// Iterate within the directory entry block pointed by direct pointer
		for(int j = 0; j< 128; j++){
			dir_ent_t dirEntry = dirEntBlock->entries[j];
			if(strcmp(dirEntry.name, name) == 0){
				m->c_received_inum = dirEntry.inum;
				return 0;
			}
		}

	}
	
	return -1;
	
}

/* This function makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified 
by pinum of name name. Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long. 
If name already exists, return success.

Information format:
*/
int run_cret(message_t* m){
	
	int pinum = m->c_sent_inum;
	int type = m->c_sent_ftype;
	printf("TYPE line 375: %d\n", type);
	char name[28];
	strcpy(name, m->c_sent_name);

	// PARENT CHECKS
	// INODE BITMAP
	// Checks if parent inode exists
	char bufBlock[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	// Checks if parent inode exist before allocating file as child
	unsigned int bitVal = get_bit((unsigned int*) bufBlock, pinum);
	if(bitVal == 0)
		return -1;

	// ETHAN: CHECK IF NAME IS ACTUALLY ALREAD IN THE PARENT DIRECTORY

	// ETHAN: CHECKS IF THERE IS ANY UNASSIGNED INODE. IF NON, RETURN HERE.


	// DATA BITMAP
	// Checks if parent data block exists
	char bufBlockDataBitMap[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlockDataBitMap, BLOCKSIZE);

	// Checks if parent directory entries exist
	unsigned int bitVal1 = get_bit((unsigned int*) bufBlock, pinum);
	if(bitVal1 == 0)
		return -1;

	// INODE TABLE
	// Retrieves parent inode from inode table using inode number indexing, since it exists
	int blockNumberOffsetInInodeTable = ceil((pinum * sizeof(inode_t))/ BLOCKSIZE);
	char bufBlock2[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock2, BLOCKSIZE);

	inode_block_t* pinodeBlockPtr = (inode_block_t*) bufBlock2;

	int remainingInodeOffset = (pinum * sizeof(inode_t)) % BLOCKSIZE;

	// parent inode retrieved
	inode_t pinode = pinodeBlockPtr->inodes[remainingInodeOffset/ sizeof(inode_t)];


	// CHILD
	//// If program reaches this point: new file with name can be created.

	// Retrieves an unallocated inode number, to be allocated to the NEW directory entry
	int newlyCreatedInodeNum = 0;
	for(int i = 0; i< SUPERBLOCKPTR->num_inodes; i++){
		unsigned int bitVal = get_bit((unsigned int*) bufBlock, i);
		if(bitVal == 0){
			newlyCreatedInodeNum = i;

			// INODE BITMAP for child
			set_bit((unsigned int*) bufBlock, i);
			break;
		}
			
	}


	// Find newly created inode using indexing of inode 
	blockNumberOffsetInInodeTable = ceil((newlyCreatedInodeNum * sizeof(inode_t))/ BLOCKSIZE);
	char bufBlock3[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock3, BLOCKSIZE);

	inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock3;

	remainingInodeOffset = (newlyCreatedInodeNum * sizeof(inode_t))% BLOCKSIZE;

	inode_t newInode = inodeBlockPtr->inodes[remainingInodeOffset/ sizeof(inode_t)];


	// Assign new inode in inode table
	newInode.type = type;
	newInode.size = 0;


	// DATA REGION
	// Checks if parent directory entries has space for new directory entry

	bool isThereEmptyDirEnt = false;
	for(int i = 0; (i< DIRECT_PTRS )&& (!isThereEmptyDirEnt); i++){
		int blockNumberToParentBlocks = pinode.direct[i];

		char bufBlockParentDirectoryEntry[BLOCKSIZE];
		lseek(fd, blockNumberToParentBlocks * BLOCKSIZE, SEEK_SET);
		read(fd, bufBlockParentDirectoryEntry, BLOCKSIZE);

		dir_block_t* dirEntBlock = (dir_block_t*) bufBlockParentDirectoryEntry;
		for(int j = 0; j< 128; j++){
			dir_ent_t* dirEntry = &dirEntBlock->entries[j];
			// There is an empty directory entry
			if(dirEntry->inum == -1){
				isThereEmptyDirEnt = true;
				dirEntry->inum = newlyCreatedInodeNum;
				strcpy(dirEntry->name, name);
				break;
			}
			
		}
	}

	// If there is no space, find a new block to be allocated to direct[]
	int newBlockDirNum = 0;
	bool isFoundNewBlock = false;
	if(!isThereEmptyDirEnt){
		
		// Search for empty data blocks in data bitmap
		for(int i = 0; i< SUPERBLOCKPTR->num_data; i++){
			
			unsigned int bitVal = get_bit((unsigned int*) bufBlockDataBitMap, i);
			if(bitVal == 0){
				newBlockDirNum = i;
				isFoundNewBlock = true;
				set_bit((unsigned int*) bufBlockDataBitMap, i);
			}
		}

		// Allocate new entry into new block number
		dir_block_t newBufBlockForDirPtrArr;

		newBufBlockForDirPtrArr.entries[0].inum = newlyCreatedInodeNum;
		strcpy(newBufBlockForDirPtrArr.entries[0].name, name);
		// newBufBlockForDirPtrArr.entries[0].name = name;
		

		lseek(fd, newBlockDirNum * BLOCKSIZE, SEEK_SET);
		write(fd, (void*)&newBufBlockForDirPtrArr, BLOCKSIZE);
	}
	if (isFoundNewBlock) {
		printf("??");
	}

	


	// If it is a directory file which we create, we have to create direct pointers to . and .. blocks in 1 data.
	if(type == MFS_DIRECTORY){
		
	// If it is a regular file
	}else{

	}

	printf("%p\n", &newInode);

	// ETHAN: IF TYPE IS DIRECTORY, THIS "NEWINODE" SHOULD HAVE DIRECT POINTERS TO ONE DATA BLOCK 4KB, WITH . ..

	// ETHAN: IF TYPE IS FILE, THIS "NEWINODE". DONT ALLOCATE BLOCK, BUT CHECK WHEN YOU ARE WRITING IF THERE IS A BLOCK THAT IS ALLOCATED.!!

	// Assign new directory entry in directory
	// emptyDirEnt->inum = newlyCreatedInodeNum;
	// strcpy(emptyDirEnt->name, name);


	// Check if it is a directory

	// DATA REGION


	fsync(fd); // Idempotency says to fsync before each successful return
	return 0;
}

/* This function removes the file or directory name from the directory specified by pinum. 0 on success, -1 on failure. Failure modes: 
pinum does not exist, directory is NOT empty. Note that the name not existing is NOT a failure by our definition (think 
about why this might be).

Information format:
*/
int run_unlink(message_t* m){
	int pinum = m->c_sent_inum;
	char name[28];
	strcpy(name, m->c_sent_name);

	// INODE BITMAP
	char bufBlock[BLOCKSIZE];
	lseek(fd, SUPERBLOCKPTR->inode_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);
	unsigned int bitVal = get_bit((unsigned int*) bufBlock, pinum);
	if(bitVal == 0)
		return -1;

	// INODE TABLE
	// Get Inode address in table
	int blockNumberOffsetInInodeTable = ceil((pinum * sizeof(inode_t))/ BLOCKSIZE);
	char bufBlock2[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock2, BLOCKSIZE);

	inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock2;
	int remainingInodeOffset = (pinum * sizeof(inode_t)) % BLOCKSIZE;

	inode_t pinode = inodeBlockPtr->inodes[remainingInodeOffset/ sizeof(inode_t)];

	for(int i = 0; i< DIRECT_PTRS; i++){
		int blockNumber = pinode.direct[i];

		char bufBlockForDirBlock[BLOCKSIZE];
		lseek(fd, blockNumber * BLOCKSIZE, SEEK_SET);
		read(fd, bufBlock, BLOCKSIZE);
		dir_block_t* dirEntBlock = (dir_block_t*) bufBlockForDirBlock;

		// Iterate within the directory entry block pointed by direct pointer
		for(int j = 0; j< 128; j++){
			dir_ent_t dirEntry = dirEntBlock->entries[j];
			if (pinode.type == 0) {
				if (dirEntry.inum == -1) {
					memcpy(dirEntry.name, "", 0);
				} else {
					return -1;
				}
			}
			else if (pinode.type == 1) {
				dirEntry.inum = -1;
				memcpy(dirEntry.name, "", 0);
			}
		}

	}
	fsync(fd);
	return 0;
}

/*This function  just tells the server to force all of its data structures to disk and shutdown by calling exit(0). This interface 
will mostly be used for testing purposes.

Information format:
*/
int run_shutdown(message_t* m){
	fsync(fd);
	close(fd);
	// m->c_received_rc = 0;
	//isShutdown = true;
	UDP_Close(PORTNUM);
	exit(0);
	return 0;
}


int message_parser(message_t* m){

	// Decode message_t 
	int message_func = m->c_sent_mtype;
	int rc;

	if(message_func == MFS_INIT){
		return 0;
		//rc = run_init(m);

	}else if(message_func == MFS_LOOKUP){
		rc = run_lookup(m);

	}else if(message_func == MFS_STAT){
		rc = run_stat(m);

	}else if(message_func == MFS_WRITE){
		rc = run_write(m);

	}else if(message_func == MFS_READ){
		rc = run_read(m);

	}else if(message_func == MFS_CRET){
		rc = run_cret(m);

	}else if(message_func == MFS_UNLINK){
		rc = run_unlink(m);

	}else if(message_func == MFS_SHUTDOWN){
		rc = run_shutdown(m);

	}

	return rc;
}

// Read image file to local memory
int readImage(){

	fd = open(IMGFILENAME, O_RDWR);
	assert(fd > -1);

	char* bufBlockPtr = (char*)malloc(sizeof(char) * BLOCKSIZE);
	read(fd, bufBlockPtr, BLOCKSIZE); //reads the first block in the image to the buffer block
	SUPERBLOCKPTR = (super_t*) bufBlockPtr;

	// struct stat sbuf;
	// int rc = fstat(fd, &sbuf);
	// assert(rc > -1);
	
	// int image_size = (int) sbuf.st_size;

	// image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    // assert(image != MAP_FAILED);

	// SUPERBLOCKPTR = (super_t *) image;

	// For testing purposes: Delete once we are confident disk is allocated properly/////////////////////////////////////
	printf("-------Describes IMG + super block-----------\n");
	//printf("IMAGE: \n");
	//printf(" IMG size created and mapped to mmap: %d\n", image_size);

	printf("SUPERBLOCK\n");
	printf(" inode bitmap address %d [len %d]\n", SUPERBLOCKPTR->inode_bitmap_addr, SUPERBLOCKPTR->inode_bitmap_len);
    printf(" data bitmap address %d [len %d]\n", SUPERBLOCKPTR->data_bitmap_addr, SUPERBLOCKPTR->data_bitmap_len);
	printf(" inode region address %d [len %d]\n", SUPERBLOCKPTR->inode_region_addr, SUPERBLOCKPTR->inode_region_len);
	printf(" data region address %d [len %d]\n", SUPERBLOCKPTR->data_region_addr, SUPERBLOCKPTR->data_region_len);
	printf(" num inodes: %d ;num data: [len %d]\n", SUPERBLOCKPTR->num_inodes, SUPERBLOCKPTR->num_data);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	return 0;
}

// server code

// Edited server main function:
int main(int argc, char *argv[]) {

	signal(SIGINT, intHandler); // Safely exit server when using ctrl + c by closing the socket during an interrupt
	
	PORTNUM = atoi(argv[1]);
	IMGFILENAME = (char*) malloc(100 * sizeof(char));
	strcpy(IMGFILENAME, argv[2]);

	// Maps the image file to local memory
	int rc = readImage();
	assert(rc > -1);

    sd = UDP_Open(PORTNUM); // Opens server socket with port 10000
    assert(sd > -1);
    while (!isShutdown) {
	struct sockaddr_in addr;

	// Create a message_t struct to store data received from client
	message_t m;
	
	// Read data from client. rc >0 denotes a successful receivetest.img from a client.
	rc = UDP_Read(sd, &addr, (char *) &m, sizeof(message_t));
	if (rc > 0) {
		rc = message_parser(&m);

		// Assigns message_t return code to see if server side operation is successful
		m.c_received_rc = rc;

		rc = UDP_Write(sd, &addr, (char *) &m, sizeof(message_t));
	} 
    }
    return 0; 
}
 
