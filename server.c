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
#include "ufs.h"
#include "udp.h"
#include "message.h"

#define BLOCKSIZE (4096)

static int PORTNUM;
static char* IMGFILENAME;

static super_t* SUPERBLOCKPTR;

typedef struct {
	inode_t inodes[UFS_BLOCK_SIZE / sizeof(inode_t)];
} inode_block_t;

typedef struct {
	dir_ent_t entries[128];
} dir_block_t;

typedef struct {
	unsigned int bits[UFS_BLOCK_SIZE / sizeof(unsigned int)];
} bitmap_t;

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

int run_init(message_t* m) {
	return 0;

}
int run_lookup(message_t* m){

	// Search in superblock

	// Search in inode bitmap
	
	
	//uintptr_t intAddrOfInodeBitMap = (uintptr_t)(SUPERBLOCKPTR-> inode_bitmap_addr * BLOCKSIZE); // integer pointer
	//unsigned int bit = get_bit((void*)SUPERBLOCKPTR + intAddrOfInodeBitMap, m->c_sent_inum);

	int offsetBytes = (SUPERBLOCKPTR-> inode_bitmap_addr * BLOCKSIZE);
	unsigned int bit = get_bit((void*)SUPERBLOCKPTR + offsetBytes, m->c_sent_inum);
	// Check if inode is not allocated/ inode not found in the disk
	if((int)bit == 0){
		m->c_received_inum = -1;
		return -1;
	}
	
	// Search in inode table if inode is found
	offsetBytes = (SUPERBLOCKPTR-> data_bitmap_addr * BLOCKSIZE);
	void* inodeTableAddr = (void*)SUPERBLOCKPTR + offsetBytes;
	

	inode_block_t* inodeBlockStruct = (inode_block_t*) inodeTableAddr;
	inode_t inode = inodeBlockStruct->inodes[SUPERBLOCKPTR-> num_inodes];
	unsigned int blockNumber;
	for(int i = 0; i< DIRECT_PTRS; i++){
		blockNumber = inode.direct[i]; // block number in the data region

		void* addressToDataRegion = SUPERBLOCKPTR + (blockNumber * BLOCKSIZE);
		dir_block_t* directoryEntryBlock = (dir_block_t*) addressToDataRegion;
		dir_ent_t directoryEntry = directoryEntryBlock-> entries[SUPERBLOCKPTR-> num_inodes];
		if(strcmp(directoryEntry.name, m->c_sent_name) == 0){
			m->c_received_inum = directoryEntry.inum;
			return 0;
		}

	}

	// Assign inum as -1 if failed to locate inode
	m->c_received_inum = -1;
	return 0;
}

int run_stat(message_t* m){
	return -1;
}

int run_write(message_t* m){
	return -1;
}

int run_read(message_t* m){
	return -1;
}

int run_cret(message_t* m){
	return -1;
}

int run_unlink(message_t* m){

	return -1;
}

int run_shutdown(message_t* m){
	return -1;
}


int message_parser(message_t* m){

	// Decode message_t 
	int message_func = m->c_sent_mtype;
	int rc;

	if(message_func == MFS_INIT){
		rc = run_init(m);

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

	int fd = open(IMGFILENAME, O_RDWR);
	assert(fd > -1);

	struct stat sbuf;
	int rc = fstat(fd, &sbuf);
	assert(rc > -1);
	
	int image_size = (int) sbuf.st_size;

	void *image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(image != MAP_FAILED);

	SUPERBLOCKPTR = (super_t *) image;

	// For testing purposes: Delete once we are confident disk is allocated properly/////////////////////////////////////
	printf("-------Describes IMG + super block-----------\n");
	printf("IMAGE: \n");
	printf(" IMG size created and mapped to mmap: %d\n", image_size);

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
	
	PORTNUM = atoi(argv[1]);
	IMGFILENAME = (char*) malloc(100 * sizeof(char));
	strcpy(IMGFILENAME, argv[2]);

	// Maps the image file to local memory
	int rc = readImage();
	assert(rc > -1);

    int sd = UDP_Open(PORTNUM); // Opens server socket with port 10000
    assert(sd > -1);
    while (1) {
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
 
