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
#include "ufs.h"
#include "udp.h"
#include "message.h"


#define BLOCKSIZE (4096)

static int PORTNUM;
static char* IMGFILENAME;

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

int run_init(message_t* m) {
	return 0;

}
int run_lookup(message_t* m){

	// Run read to get MFS_DirEnt_t
	run_read(m);
	MFS_DirEnt_t dirEntry = m->c_received_mfs_dirent;
	m->c_received_inum = dirEntry.inum;
	m->c_received_rc = 0;
	
	return 0;
}

int run_stat(message_t* m){

	//MFS_Stat_t stat;

	// Search for the file/ directory of the inode number


	return -1;
}

int run_write(message_t* m){
	return -1;
}

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

	// // INODE TABLE
	// // Gets inode table's location
	// lseek(fd, SUPERBLOCKPTR-> inode_region_addr * BLOCKSIZE, SEEK_SET);
	// read(fd, bufBlock, BLOCKSIZE);

	// // Read inode table block
	// inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock; 
	
	// // Read inode table and obtain inode
	// inode_t inode = inodeBlockPtr->inodes[inum];

	// DATA BITMAP
	// Check whether the data region exists a data allocation for inode
	lseek(fd, SUPERBLOCKPTR->data_bitmap_addr * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	bitVal = get_bit((unsigned int*) bufBlock, inum);

	// Check if inode data is allocated 
	if(bitVal == 0)
		return -1;

	// DATA REGION
	m->c_received_buffer_size = nbytes; // Defines nbytes to decode in client
	lseek(fd, offset, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE); // Takes BLOCKSIZE of data, but actually only needs nbytes.
	strcpy(m->c_received_buffer, bufBlock);

	dir_block_t* dirEntryBlock = (dir_block_t*) bufBlock;

	for(int i = 0; i< DIRECT_PTRS; i++){
		dir_ent_t dirEntry = dirEntryBlock->entries[i];
		if(dirEntry.inum == inum){
			m->c_received_mfs_dirent.inum = dirEntry.inum;
			strcpy(m->c_received_mfs_dirent.name, dirEntry.name);

			m->c_received_rc = 0;
		}

	}

	
	return 0;
}

int run_cret(message_t* m){
	return -1;
}

int run_unlink(message_t* m){

	return -1;
}

int run_shutdown(message_t* m){
	fsync(fd);
	close(fd);
	m->c_received_rc = 0;
	exit(0);
	return -0;;
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
 
