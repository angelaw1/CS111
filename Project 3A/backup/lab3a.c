// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "ext2_fs.h"

#define BUFFSIZE          256
#define EXIT_FAIL         1
#define EXIT_SUCCESS      0
#define SUPERBLOCKSIZE    sizeof(struct ext2_super_block)
#define SUPERBLOCKOFFSET  1024
#define BLOCKDESCSIZE			sizeof(struct ext2_group_desc)
#define BLOCKDESCOFFSET		SUPERBLOCKSIZE + SUPERBLOCKOFFSET

int input_fd;
char *file;
struct ext2_super_block superblock;
struct ext2_group_desc block_descriptor;

// prints error message and exits
void syscall_error(void) {
	fprintf(stderr, "Error number: %d\n %s\n", errno, strerror(errno));
	exit(EXIT_FAIL);
}

void usage_error(void) {
	fprintf(stderr, "Correct Usage: ./lab4b filename\n");
	exit(EXIT_FAIL);
}

void superblockSumm() {
	  if (pread(input_fd, &superblock, SUPERBLOCKSIZE, SUPERBLOCKOFFSET) == -1) {
	    syscall_error();
	  }

		__u32 totalBlocks = superblock.s_blocks_count;
		__u32 totalInodes = superblock.s_inodes_count;
		__u32 bsize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
		__u16 inodeSize = superblock.s_inode_size;
		__u32 bpergroup = superblock.s_blocks_per_group;
		__u32 ipergroup = superblock.s_inodes_per_group;
		__u32 firstinodeBlock = superblock.s_first_ino;

		printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
					totalBlocks,
					totalInodes,
					bsize,
					inodeSize,
					bpergroup,
					ipergroup,
					firstinodeBlock);
}

void groupSumm() {
	if (pread(input_fd, &block_descriptor, BLOCKDESCSIZE, BLOCKDESCOFFSET) == -1) {
		syscall_error();
	}

	__u32 totalBlocksIn_groupNum = superblock.s_blocks_count;
	__u32 totalInodesIn_groupNum = superblock.s_inodes_count;

	if (totalBlocksIn_groupNum > superblock.s_blocks_per_group) {
		totalBlocksIn_groupNum = superblock.s_blocks_per_group;
	}
	if (superblock.s_inodes_count > superblock.s_inodes_per_group) {
		totalBlocksIn_groupNum = superblock.s_inodes_per_group;
	}
	int groupNum = 0;
	__u16 totalFreeBlocks = block_descriptor.bg_free_blocks_count;
	__u16 totalFreeInodes = block_descriptor.bg_free_inodes_count;
	__u32 blockbitmapBlock = block_descriptor.bg_block_bitmap;
	__u32 inodebitmapBlock = block_descriptor.bg_inode_bitmap;
	__u32 firstFreeInodeBlock = block_descriptor.bg_inode_table;
	printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
				groupNum,
				totalBlocksIn_groupNum,
				totalInodesIn_groupNum,
				totalFreeBlocks,
				totalFreeInodes,
				blockbitmapBlock,
				inodebitmapBlock,
				firstFreeInodeBlock);
}

void freeBlockEntries() {

}

void freeInodeEntries() {

}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage_error();
	}

	if ((file = malloc(strlen(argv[1]))) == NULL) {
		syscall_error();
	}
	file = argv[1];

  input_fd = open(file, O_RDONLY);
  if (input_fd == -1) {
    syscall_error();
  }

	superblockSumm();
	groupSumm();
}
