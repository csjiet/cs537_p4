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

// Takes in a bitmap, offsets starting from the right, offset by "position" and the pointed next bit will change to 1
// E.g., 100000 -(bitmap, 2)-> 100100
// E.g., 100100 -(bitmap, 2)-> 100100
// E.g., 100100 -(bitmap, 3)-> 110100
void set_bit_to_one(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
   //bitmap[index] &= ~(0x1 << offset);
   
}
// Takes in a bitmap, offsets starting from the right, offset by "position" and the pointer next bit will change to 0
// E.g., 100000 -(bitmap, 2)-> 100000
// E.g., 100100 -(bitmap, 2)-> 100000
void set_bit_to_zero(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   // bitmap[index] |= 0x1 << offset;
   bitmap[index] &= ~(0x1 << offset);
   
}

int getBitmapValGivenBlockNumAndInum(int blockNumberAddress, int inum){
	// printf("___________________________\n");
	// printf("GetBitmapVal\n");
	// printf("blockNumAddr: %d\n", blockNumberAddress);
	// printf("inum: %d\n", inum);
	char bufBlock[BLOCKSIZE];
	lseek(fd, blockNumberAddress * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	//inode_block_t* inodeBlock = (inode_block_t*) bufBlock;

	//printf("bit: %d\n", get_bit((unsigned int*) bufBlock, inum));	

	return get_bit((unsigned int*) bufBlock, inum);
	
}

int setOneToBitMap(int blockNumberAddress, int inum){
	// printf("setBitmapValGivenBlockNumAndInum\n");
	// printf("blockNumAddr: %d\n", blockNumberAddress);
	// printf("inum: %d\n", inum);
	
	char bufBlock[BLOCKSIZE];
	//int bufBlock[BLOCKSIZE/sizeof(int)];

	lseek(fd, blockNumberAddress * BLOCKSIZE, SEEK_SET);
	
	read(fd, bufBlock, BLOCKSIZE);

	
	set_bit_to_one((unsigned int*) bufBlock, inum);

	lseek(fd, blockNumberAddress * BLOCKSIZE, SEEK_SET);
	write(fd, bufBlock, BLOCKSIZE);

	return 0;
	
}

int setZeroToBitMap(int blockNumberAddress, int inum){
	// printf("setBitmapValGivenBlockNumAndInum\n");
	// printf("blockNumAddr: %d\n", blockNumberAddress);
	// printf("inum: %d\n", inum);
	
	char bufBlock[BLOCKSIZE];
	//int bufBlock[BLOCKSIZE/sizeof(int)];

	lseek(fd, blockNumberAddress * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	
	set_bit_to_zero((unsigned int*) bufBlock, inum);

	lseek(fd, blockNumberAddress * BLOCKSIZE, SEEK_SET);
	write(fd, bufBlock, BLOCKSIZE);

	return 0;
	
}

void visualizeInode(inode_t* inode){

	printf("~~~~~~Inode_t inode visualization!!!~~~~~~~~~\n");
	printf("inode.type (0 is directory): %d\n",inode->type);
	printf("----\n");
	printf("inode.size: %d\n",inode->size);
	printf("----\n");
	for(int i = 0; i< DIRECT_PTRS; i++){
		printf("direct[%d]: %d\n", i,inode->direct[i]);
	}
}

int getInodeCopyFromInodeTable(int inum, inode_t* inode){
	// printf("----------------------\n");
	// printf("parameters - INUM: %d, inodeType BEFORE UPDATE: %d, inodeSize BEFORE UPDATE: %d\n", inum, inode->type, inode->size);
	int blockNumberOffsetInInodeTable = ceil((inum * sizeof(inode_t))/ BLOCKSIZE);
	// printf("BlockNumberOffset: %d\n", blockNumberOffsetInInodeTable);

	char bufBlock[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	inode_block_t* inodeBlockPtr = (inode_block_t*) bufBlock;

	int remainingInodeOffset = (inum * sizeof(inode_t)) - (blockNumberOffsetInInodeTable * BLOCKSIZE);
	// printf("RemainingOffset: %d\n", remainingInodeOffset);
	
	// Inode retrieved
	inode_t inumInode = inodeBlockPtr->inodes[(int)(remainingInodeOffset/ sizeof(inode_t))];
	// printf("inode_block index: %d\n",(int)(remainingInodeOffset/ sizeof(inode_t)));
	inode->type = inumInode.type;
	inode->size = inumInode.size;

	// printf("inode_t AFTER retrieval: inode-> type: %d; inode->size: %d\n", inode->type, inode->size);
	// printf("Items in inode AFTER retrieval: \n");
	for(int i = 0; i< DIRECT_PTRS; i++){
		inode->direct[i] = inumInode.direct[i];

	}

	//visualizeInode(&inumInode);

	// printf("size of inode_t: %ld\n", sizeof(inode_t));
	// printf("Retrieved inode: inode_t -> type: %d, ->size: %d\n", inode->type, inode->size);
	// printf("direct[] in inode_t:\n");
	// for(int i = 0; i< DIRECT_PTRS; i++){
	// 	// Prints out bitval: not alloced if direct[i] points to -1
	// 	if(inode->direct[i] == -1){
	// 		printf("item %d>> %d : bitval: not alloced\n", i, inode->direct[i]);

	// 	}else{
	// 		printf("item %d>> %d : bitval: %d\n", i, inode->direct[i], getBitmapValGivenBlockNumAndInum(inode->direct[i], inum));
	// 	}
		

	// }

	return 0;
}

int getDataForDirectoryEntryBlockCopyFromDataRegion(int blockNumberOffset, dir_block_t* dirEntryBlock){

	char bufBlock[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->data_region_addr + blockNumberOffset) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	dir_block_t* foundDirEntryBlock = (dir_block_t*) bufBlock;

	for(int i = 0; i< 128; i++){
		// printf("dirEntryBlock->entries[%d]: entries[]-> inum: (%d); entries[]-> name: (%s) \n",i, dirEntryBlock->entries[i].inum, dirEntryBlock->entries[i].name);

		dirEntryBlock->entries[i] = foundDirEntryBlock->entries[i];
	}

	return 0;

}

int getDataForNormalBlockCopyFromDataRegion(int blockNumberOffset, char* dirEntryBlock){

	char bufBlock[BLOCKSIZE];
	lseek(fd, (SUPERBLOCKPTR->data_region_addr + blockNumberOffset) * BLOCKSIZE, SEEK_SET);
	read(fd, bufBlock, BLOCKSIZE);

	for(int i = 0; i< BLOCKSIZE; i++){
		dirEntryBlock[i] = bufBlock[i];
	}

	return 0;

}

int getFreeInodeCopyFromInodeTable(int* inum, inode_t* inode){

	// printf("______________________________________\n");
	// printf("parameters BEFORE UPDATE: inum: %d; inode->type: %d; inode->size: %d\n", *inum, inode->type, inode->size);
	// printf("Items in direct[]:\n");
	// for(int i = 0 ; i< DIRECT_PTRS; i++){
	// 	printf("item %d) %d\n", i, inode->direct[i]);
	// }
	// printf("SUPERBLOCKPTR->num_inodes: %d\n", SUPERBLOCKPTR->num_inodes);

	// INODE BITMAP to get unallocated inode number
	int unallocatedInodeNumber = 0;

	for(int i = 0; i< SUPERBLOCKPTR->num_inodes; i++){
		unsigned int bitVal = getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->inode_bitmap_addr, i);
		// printf("BitVal: %d\n", bitVal);
		if(bitVal == 0){
			unallocatedInodeNumber = i;
			//printf("unallocatedNumber: %d\n", unallocatedInodeNumber);

			// Set newly found unallocated inode bit as allocated/ 1
			setOneToBitMap(SUPERBLOCKPTR->inode_bitmap_addr, i);

			//printf("Setting bit val at inode bitmap to 1 after obtained free inode_t - bitval (Should be 1): %d\n", getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->inode_bitmap_addr, i));

			// INODE TABLE
			// Find newly created inode using indexing of inode 
			// Progagate into inode passed in as argument
			int rc = getInodeCopyFromInodeTable(unallocatedInodeNumber, inode);
			if (rc < 0)
				return -1;
			*inum = unallocatedInodeNumber;
			// printf("parameters AFTER UPDATE: inum: %d; inode->type: %d; inode->size: %d\n", *inum, inode->type, inode->size);
			// printf("items in free new inode's direct[] AFTER obtaining inode_t:\n");
			// for(int j = 0; j< DIRECT_PTRS; j++){
			// 	printf("Items %d >>> %d\n", j, inode->direct[j]);
			// }

			return 0;
		}
	}
	
	
	return -1;
}

int getFreeDirectoryEntryDataBlockCopyFromDataRegion(int* blockNumber, dir_block_t* bufferBlock){


	// printf("Entering getFreeDataBlockCopyFromDataRegion(): ---------------------\n");
	// printf("param BEFORE UPDATE -- blockNumber: %d, dir_block_t* dirEntryBlock: %p\n", *blockNumber, bufferBlock);
	// DATA BITMAP
	int unallocatedDatablockNumber = 0;

	for(int i = 0; i< SUPERBLOCKPTR->num_data; i++){
		unsigned int bitVal = getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->data_bitmap_addr, i);
		
		if(bitVal == 0){
			unallocatedDatablockNumber = i;
			// printf("unallocted data block number: %d\n", unallocatedDatablockNumber);
			// printf("bitVal of found FREE data block (should be 0): %d\n", bitVal);
			// set the found data block as allocated in data bitmap/ 1
			setOneToBitMap(SUPERBLOCKPTR->data_bitmap_addr, i);

			//printf("bitVal of AFTER set to 1 (should be 1): %d\n", getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->data_bitmap_addr, i));

			//getDataForDirectoryEntryBlockCopyFromDataRegion(unallocatedDatablockNumber + SUPERBLOCKPTR->data_region_addr, (dir_block_t*) bufferBlock);
			getDataForDirectoryEntryBlockCopyFromDataRegion(unallocatedDatablockNumber, (dir_block_t*) bufferBlock);

			*blockNumber = unallocatedDatablockNumber;

			return 0;
		}
	}
	
	return -1;
}

int getFreeNormalDataBlockCopyFromDataRegion(int* blockNumber, char* bufferBlock){


	printf("Entering getFreeNormalDataBlockCopyFromDataRegion(): ---------------------\n");
	printf("param BEFORE UPDATE -- blockNumber: %d, dir_block_t* dirEntryBlock: %p\n", *blockNumber, bufferBlock);
	// DATA BITMAP
	int unallocatedDatablockNumber = 0;

	for(int i = 0; i< SUPERBLOCKPTR->num_data; i++){
		unsigned int bitVal = getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->data_bitmap_addr, i);
		
		if(bitVal == 0){
			unallocatedDatablockNumber = i;
			printf("unallocted data block number: %d\n", unallocatedDatablockNumber);
			printf("bitVal of found FREE data block (should be 0): %d\n", bitVal);
			// set the found data block as allocated in data bitmap/ 1
			setOneToBitMap(SUPERBLOCKPTR->data_bitmap_addr, i);

			printf("bitVal of AFTER set to 1 (should be 1): %d\n", getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->data_bitmap_addr, i));

			
			getDataForNormalBlockCopyFromDataRegion(unallocatedDatablockNumber + SUPERBLOCKPTR->data_region_addr, bufferBlock);

			*blockNumber = unallocatedDatablockNumber;

			return 0;
		}
	}
	
	return -1;
}

int addInodeToInodeTable(int inum, inode_t* inode){

	int blockNumberOffsetInInodeTable = ceil((inum * sizeof(inode_t))/ BLOCKSIZE);
	int remainingOffsetWithinABlock = (inum * sizeof(inode_t)) - (blockNumberOffsetInInodeTable * BLOCKSIZE);

	// printf("blockNumber offset in inode table: %d\n", blockNumberOffsetInInodeTable);
	// printf("remaining offset within a block: %d\n", remainingOffsetWithinABlock);

	int offset = ((SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE) + remainingOffsetWithinABlock;

	// printf("offset in lseek to inode_t immediately: %d\n", offset);
	lseek(fd, offset, SEEK_SET);

	// printf("inode_t passed in inode->type: %d\n", inode->type);
	// printf("inode_t passed in inode->size: %d\n", inode->size);
	// printf("items in free new inode's direct[] AFTER obtaining inode_t:\n");
	// for(int j = 0; j< DIRECT_PTRS; j++){
	// 	printf("Items %d >>>> %d\n", j, inode->direct[j]);
	// }

	write(fd, inode, sizeof(inode_t));

	return 0;

}

int addDirectoryEntryBlockToDataRegion(int blockNumber, dir_block_t* dirEntryBlock){

	lseek(fd, (SUPERBLOCKPTR->data_region_addr + blockNumber) * BLOCKSIZE, SEEK_SET);
	write(fd, dirEntryBlock, sizeof(dir_block_t));

	return 0;

}

int findParentNameGivenInum(int pinum, char* name){

	int blockNumberOffsetInInodeTable = ceil((pinum * sizeof(inode_t))/ BLOCKSIZE);
	int remainingOffsetWithinABlock = ceil((pinum * sizeof(inode_t))% BLOCKSIZE);

	char bufBlock[sizeof(inode_t)];
	lseek(fd, (SUPERBLOCKPTR->inode_region_addr + blockNumberOffsetInInodeTable) * BLOCKSIZE + remainingOffsetWithinABlock, SEEK_SET);
	read(fd, bufBlock, sizeof(inode_t));

	inode_t* inodeBlockPtr = (inode_t*) bufBlock;

	int firstBlockOfDirectBlockPtr = inodeBlockPtr->direct[0];

	char dirBlockBuf[sizeof(dir_block_t)];
	lseek(fd, firstBlockOfDirectBlockPtr * BLOCKSIZE, SEEK_SET);
	read(fd, dirBlockBuf, sizeof(dir_block_t));

	dir_block_t* dirBlock = (dir_block_t*) dirBlockBuf;
	dir_ent_t parentDirEntry = dirBlock->entries[0];

	strcpy(name, parentDirEntry.name);

	return 0;

}

void visualizeDirBlock(dir_block_t* dirBlock){

	printf("~~~~~~~~~~~~~~~~dir_block_t VISUALIZER~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	for(int i = 0; i< 128; i++){
		dir_ent_t dirEntry = dirBlock->entries[i];
		printf("dir_ent_t entries[%d].inum: %d   ;", i, dirEntry.inum);
		printf("dir_ent_t entries[%d].name: %s\n", i, dirEntry.name);
	}

}


int addDirEntryToDirectoryInode(inode_t* dinode, int dinum, inode_t* addedInode, dir_ent_t* copyOfDirEntryToAdd){

	// printf("START OF addDirEntryToDirectoryInode() param:\n");
	// printf("param 1 -- inode_t dinode.type: %d; inode_t dinode.size: %d\n", dinode.type, dinode.size);
	// printf("param 2 -- int dinum: %d\n", dinum);
	// printf("param 3 -- inode_t dinode.type: %d; inode_t dinode.size: %d\n", addedInode.type, addedInode.size);
	// printf("param 4 -- dir_ent_t name: %s; dir_ent_t inum: %d\n", copyOfDirEntryToAdd.name, copyOfDirEntryToAdd.inum);

	// Ensure inode is a directory
	assert(dinode->type == MFS_DIRECTORY);

	// Check if directory entry dir_ent_t can be add in the current blocks we have in direct[]
	bool isThereEmptyDirEnt = false;

	// Buffer to store each block pointed by direct pointer
	char bufBlock[BLOCKSIZE];

	int lastIndexInDirectPtrArr = 0; // This variable holds the latest unallocated block, in case new block allocation is needed if all existing blocks are full
	for(int i = 0; i< DIRECT_PTRS && (!isThereEmptyDirEnt); i++){
		int blockNumber = dinode->direct[i];
		// Check if blockNumber is zero, if it is that means you ran out of blocks in your inode_t directory
		if(blockNumber == 0 || blockNumber == -1){
			// printf("blockNumber: %d\n", blockNumber);
			// printf("WUTTWUTWTUWTUWTUWUTWUTWUTUWTUWTUWUTWUTUWTUWUTUWUTWUTUWTUWEUWUTUWTUWTUWTUWUTWUT\n");
			break;
		}

		lseek(fd, blockNumber * BLOCKSIZE, SEEK_SET);
		read(fd, bufBlock, BLOCKSIZE);
		dir_block_t* dirEntBlock = (dir_block_t*) bufBlock;
		for(int j = 0; j< 128; j++){

			dir_ent_t dirEntry = dirEntBlock->entries[j];
			lastIndexInDirectPtrArr = i;

			// printf("Name of directory entries: %s\n", dirEntry.name);
			// Check if there are any unallocated directory entry dir_ent_t
			if(dirEntry.inum == -1){
				isThereEmptyDirEnt = true;
				// printf("Things that are stored - Item at direct[i] i - index: %d\n", j);
				// printf("Filled up dir_ent_t in this block: %d\n", blockNumber);

				// Write the dir_ent_t to disk
				// Prepare the dir_ent_t we want to write
				dirEntry.inum = copyOfDirEntryToAdd->inum;
				strcpy(dirEntry.name, copyOfDirEntryToAdd->name);

				int offsetToDirEntry = (blockNumber * BLOCKSIZE) + (j * sizeof(dir_ent_t));
				lseek(fd, offsetToDirEntry, SEEK_SET);
				write(fd, &dirEntry, sizeof(dir_ent_t));
				
				// THIS IS PRINTING TOOL VERY GOOD!
				// visualizeDirBlock(dirEntBlock);
				
				// printf("----- Found unallocated directory entry in existing blocks ------\n");
				// printf("dinode direct[]'s block number where the free dirEntry is found: %d\n", blockNumber);
				// printf("Entry number in the dir_block_t's blockNumber (Must be larger than 1 to account . & .. ): %d\n", j);
				// printf("dirEntry.inum: %d\n", dirEntry.inum);
				// printf("dirEntry.name: %s\n", dirEntry.name);
				// printf("offset to directory entry using lseek: %d\n", offsetToDirEntry);

				break;
			}
		}
		// THIS IS PRINTING TOOL VERY GOOD!
		// printf("BLOCK NUMBER FOR THIS VISUALIZED dir_block_t BLOCK: %d\n", blockNumber);
		// visualizeDirBlock(dirEntBlock);
	}

	// Check if dir_ent_t is allocated in the above blocks, if not, create a new block and add dir_ent_t
	if(!isThereEmptyDirEnt){
		// printf("REACH HERE KE??????????????????????????\n");
		// Find new data block
		int newDataBlockNumber;
		dir_block_t newDirEntryBlock;
		getFreeDirectoryEntryDataBlockCopyFromDataRegion(&newDataBlockNumber, &newDirEntryBlock);

		//printf("Found free data block for dir_block_. Block number: %d\n", newDataBlockNumber);

		// Write the new dir_ent_t to the first entry in new data block
		newDirEntryBlock.entries[0].inum = copyOfDirEntryToAdd->inum;
		strcpy(newDirEntryBlock.entries[0].name, copyOfDirEntryToAdd->name);
		
		// Write the newly found data block into disk
		lseek(fd, newDataBlockNumber * BLOCKSIZE, SEEK_SET);
		write(fd, &newDirEntryBlock, sizeof(dir_block_t));


		// printf("BLOCK NUMBER FOR THIS VISUALIZED dir_block_t BLOCK: %d\n", newDataBlockNumber);
		// visualizeDirBlock(&newDirEntryBlock);

		// Write newly allocated block number to direct[] for parent inode
		// printf("Direct[] index for new block: %d\n", lastIndexInDirectPtrArr + 1);
		dinode->direct[lastIndexInDirectPtrArr + 1] = newDataBlockNumber;

	}

	// printf("new added child inode_t type (where 0=(MFS_DIRECTORY)): %d\n", addedInode.type);

	if(addedInode->type == MFS_DIRECTORY){

		int newBlockNumberForDirectory = 0;
		dir_block_t newDirEntryBlock;
		getFreeDirectoryEntryDataBlockCopyFromDataRegion(&newBlockNumberForDirectory, &newDirEntryBlock);

		newDirEntryBlock.entries[1].inum = dinum;
		strcpy(newDirEntryBlock.entries[1].name, "..");

		newDirEntryBlock.entries[0].inum = copyOfDirEntryToAdd->inum;
		strcpy(newDirEntryBlock.entries[0].name, ".");

		addDirectoryEntryBlockToDataRegion(newBlockNumberForDirectory, &newDirEntryBlock);

		addedInode->direct[0] = newBlockNumberForDirectory;
		addInodeToInodeTable(copyOfDirEntryToAdd->inum, addedInode);
		//visualizeDirBlock(&newDirEntryBlock);

	}

	return 0;
}

/*This function returns some information about the file specified by inum. Upon success, return 0, 
otherwise -1. The exact info returned is defined by MFS_Stat_t. Failure modes: inum does not exist. 
File and directory sizes are described below.*/

int run_stat(message_t* m){
	
	int inum = m->c_sent_inum;

	// INODE BITMAP
	// Check if inum exists in inode table
	if(getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->inode_bitmap_addr, inum) == 0)
		return -1;

	// INODE TABLE
	inode_t inode;
	int rc = getInodeCopyFromInodeTable(inum, &inode);
	if(rc < 0)
		return -1;

	m->c_received_mfs_stat.size = inode.size;
	m->c_received_mfs_stat.type = inode.type;

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
	char *buffer = strdup(m->c_sent_buffer);
	
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
		if ((SUPERBLOCKPTR->inode_region_addr + offset + nbytes) < 4096) {
			// //Write to one block
			// lseek(fd, (SUPERBLOCKPTR->data_bitmap_addr) * BLOCKSIZE, SEEK_SET);
			
			// //Write data block
			// write(fd, buffer, 4096);
			
			// //Update Inode region?
			// lseek(fd, (SUPERBLOCKPTR->inode_bitmap_addr) * BLOCKSIZE, SEEK_SET);
			// write(fd, &inode, sizeof(inode_t));
		}	
		else {
			//Write to two blocks
			//Split data to write
			// int split = BLOCKSIZE - (offset % BLOCKSIZE + nbytes);
			// int block1 = SUPERBLOCKPTR->data_bitmap_addr;
			// int block2 = SUPERBLOCKPTR->data_bitmap_addr + 1;
			// lseek(fd, block1 * BLOCKSIZE, SEEK_SET);
			// printf("%d, %d\n", split, block2);
			//write(fd, buffer)
		}
	}
	
	fsync(fd); // Idempotency from writeup
	free(buffer);
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
	if(getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->inode_bitmap_addr, pinum) == 0)
		return -1;

	// DATA BITMAP
	if(getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->data_bitmap_addr, pinum) == 0)
		return -1;

	// INODE TABLE
	inode_t pinode;
	int rc = getInodeCopyFromInodeTable(pinum, &pinode);
	if(rc < 0)
		return -1;

	//visualizeInode(&pinode);

	// printf("pinum: %d\n", pinum);
	// printf("pinode.type: %d\n", pinode.type);
	if(pinode.type == MFS_REGULAR_FILE){
		m->c_received_inum = -1;
		return -1;
	}

	char bufBlock[BLOCKSIZE];
	for(int i = 0; i< DIRECT_PTRS; i++){
		int blockNumber = pinode.direct[i];

		if(blockNumber == 0 || blockNumber == -1){
			break;
		}

		lseek(fd, blockNumber * BLOCKSIZE, SEEK_SET);
		read(fd, bufBlock, BLOCKSIZE);
		dir_block_t* dirEntBlock = (dir_block_t*) bufBlock;

		for(int j = 0; j< 128; j++){
			dir_ent_t dirEntry = dirEntBlock->entries[j];
			if(strcmp(dirEntry.name, name) == 0){
				m->c_received_inum = dirEntry.inum;
				break;
			}
		}
	}
	
	return 0;
	
}



/* This function makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified 
by pinum of name name. Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long. 
If name already exists, return success.

Information format:
*/
int run_cret(message_t* m){
	// printf("START OF CREATE\n");
	int pinum = m->c_sent_inum;
	int type = m->c_sent_ftype;
	
	char name[28];
	strcpy(name, m->c_sent_name);

	// PARENT CHECKS
	// INODE BITMAP
	// Checks if parent inode exist before allocating file as child
	if(getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->inode_bitmap_addr, pinum) == 0)
		return -1;

	//printf("type: %d\n", type);

	// // DATA BITMAP
	// // Checks if parent data block exists
	if(getBitmapValGivenBlockNumAndInum(SUPERBLOCKPTR->data_bitmap_addr, pinum) == 0)
		return -1;
	

	// INODE TABLE
	// Retrieves parent inode from inode table using inode number indexing, since it exists
	inode_t pinode;
	int rc = getInodeCopyFromInodeTable(pinum, &pinode);
	if(rc < 0)
		return -1;

	
	// CHILD
	// Retrieves an unallocated inode number, to be allocated to the NEW directory entry
	int newInodeNumber;
	inode_t newInode;
	rc = getFreeInodeCopyFromInodeTable(&newInodeNumber, &newInode);
	if(rc < 0)
		return -1;

	// Assign new inode in inode table
	newInode.type = type;
	newInode.size = 0;

	addInodeToInodeTable(newInodeNumber, &newInode);

	// DATA REGION
	dir_ent_t dirEntry;
	dirEntry.inum = newInodeNumber;
	strcpy(dirEntry.name, name);

	// printf("THIS IS AT RUN_CRET()- dir_ent_t that is added as dirEntry - dirEntry.inum: %d, dirEntry.name: %s\n", dirEntry.inum, dirEntry.name);

	// Checks if parent directory entries has space for new directory entry
	addDirEntryToDirectoryInode(&pinode, pinum, &newInode, &dirEntry);

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
 
