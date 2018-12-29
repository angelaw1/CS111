// NAME: Angela Wu,Candice Zhang
// EMAIL: angelawu.123456789@gmail.com,candicezhang1997@gmail.com
// ID: 604763501,604757623

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include "ext2_fs.h"

#define BUFFSIZE          	256
#define EXIT_FAIL         	1
#define EXIT_SUCCESS      	0

int input_fd;
char *file;
struct ext2_super_block superblock;
struct ext2_group_desc block_descriptor;
uint8_t *inodeBitmap;

int superBlockOffset;
int superBlockSize;
int blockDescOff;
int blockDescSize;
int blockBitmapOffset;
int blockBitmapSize;
int inodeBitmapOffset;
int inodeBitmapSize;
int inodeTableOffset;

__u32 block_size;
__u32 inodes_count;
__u32 first_inode_block;
__u16 inode_size;

uint64_t total_offset;

// prints error message and exits
void syscall_error(void) {
    fprintf(stderr, "Error number: %d\n %s\n", errno, strerror(errno));
    exit(EXIT_FAIL);
}

void usage_error(void) {
    fprintf(stderr, "Correct Usage: ./lab3a filename\n");
    exit(EXIT_FAIL);
}

void format_time(uint32_t timestamp, char* buf) {
    time_t epoch = timestamp;
    struct tm ts = *gmtime(&epoch);
    strftime(buf, 80, "%m/%d/%y %H:%M:%S", &ts);
}

void superblockSumm() {
    superBlockOffset = 1024;
    superBlockSize = sizeof(struct ext2_super_block);
    if (pread(input_fd, &superblock, superBlockSize, superBlockOffset) == -1) {
        syscall_error();
    }
    block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;

    __u32 totalBlocks = superblock.s_blocks_count;
    __u32 totalInodes = superblock.s_inodes_count;
    __u32 bsize = block_size;
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
    blockDescOff = superBlockOffset + superBlockSize;
    blockDescSize = block_size;		// block group descriptor table only contains one group
    if (pread(input_fd, &block_descriptor, blockDescSize, blockDescOff) == -1) {
        syscall_error();
    }

    __u32 totalBlocksIn_groupNum = superblock.s_blocks_count;
    __u32 totalInodesIn_groupNum = superblock.s_inodes_count;

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
    blockBitmapOffset = block_size * block_descriptor.bg_block_bitmap;
    blockBitmapSize = block_size;

    uint8_t blockBitmap[blockBitmapSize];
    if (pread(input_fd, blockBitmap, blockBitmapSize, blockBitmapOffset) == -1) {
        syscall_error();
    }

    for (int i = 0; i < blockBitmapSize; i++) {
        for (int j = 0; j < 8; j++) {
            if ((blockBitmap[i] & (1 << j)) == 0) {
                printf("BFREE,%d\n", (i * 8) + j + 1);
            }
        }
    }
}

void freeInodeEntries() {
    inodeBitmapOffset = blockBitmapOffset + blockBitmapSize;
    inodeBitmapSize = block_size;

    inodeBitmap = malloc(inodeBitmapSize);
    if (pread(input_fd, inodeBitmap, inodeBitmapSize, inodeBitmapOffset) == -1) {
        syscall_error();
    }

    for (int i = 0; i < blockBitmapSize; i++) {
        for (int j = 0; j < 8; j++) {
            if ((inodeBitmap[i] & (1 << j)) == 0) {
                printf("IFREE,%d\n", (i * 8) + j + 1);
            }
        }
    }
}

void print_entry(__u32 inum,__u32 block_num){
    off_t db_offset = 1024 + (block_num-1)*block_size;  //data block offset
    unsigned char buf[block_size];  //buffer for the data block
    pread(input_fd,buf, block_size,db_offset);
    struct ext2_dir_entry* entry = (struct ext2_dir_entry *) buf;   //first entry in data block
    __u32 entry_offset = 0;
    while (entry_offset < block_size){
        char file_name[EXT2_NAME_LEN+1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;
        if (entry->inode != 0){
            fprintf(stdout, "DIRENT,%u,%lu,%u,%u,%u,'%s'\n",inum,total_offset,entry->inode,entry->rec_len,entry->name_len,file_name);
        }
        entry_offset += entry->rec_len;
        total_offset+=entry->rec_len;
        entry = (void*) entry + entry->rec_len;
    }
}

void single_indirect_block(__u32 inum,__u32 sib_num){
    off_t sib_offset = 1024 + (sib_num-1)*block_size; //singly indirect block offset
    __u32 buf[block_size];
    pread(input_fd,buf,block_size,sib_offset);
    for (__u32 i = 0; i < block_size/4; i++){
        __u32 block_num = buf[i];
        if (block_num != 0){
            print_entry(inum,block_num);
        }
    }
}

void double_indirect_block(__u32 inum, __u32 dib_num){
    off_t dib_offset = 1024+(dib_num-1)*block_size;
    __u32 buf[block_size];
    pread(input_fd,buf,block_size,dib_offset);
    for (__u32 i = 0; i < block_size/4; i++){
        __u32 sib_num = buf[i];
        single_indirect_block(inum, sib_num);
    }
}

void print_dir(__u32 inum,struct ext2_inode* ip){
    /*read the inode with given inum*/
    struct ext2_inode inode = *ip;
    for (int i = 0; i < 12; i++){
        __u32 block_num = inode.i_block[i]; //data block index
        if (block_num != 0){
            print_entry(inum,block_num);
        }
    }
    //deals with the 13th singly indirect block
    __u32 sib_num = inode.i_block[12];  //singly indirect block number
    if (sib_num != 0){  //if singly indirect block not empty
        single_indirect_block(inum,sib_num);
    }
    //deals with the 14th doubly indirect block
    __u32 dib_num = inode.i_block[13];
    if (dib_num != 0){
        double_indirect_block(inum,dib_num);
    }
    //deals with the 15th triple indirect block
    __u32 tib_num = inode.i_block[14];
    if (tib_num != 0){
        off_t tib_offset = 1024+(tib_num-1)*block_size;
        __u32 buf[block_size];
        pread(input_fd,buf,block_size,tib_offset);
        for (__u32 i = 0; i < block_size/4; i++){
            __u32 dib_num = buf[i];
            double_indirect_block(inum,dib_num);
        }
    }
}

void inode_summary() {
	__u32	data_block1[block_size];
	__u32	data_block2[block_size];
	__u32	data_block3[block_size];
	// int count = 0;
	int singleOffset = 12;
	int doubleOffset = singleOffset + 256;					// 256 = size of block (bytes)/size of int (bytes)
	int tripleOffset = doubleOffset + 256*256;

    inodes_count = superblock.s_inodes_count;	// total number of inodes
    first_inode_block = block_descriptor.bg_inode_table;	// block number
    inode_size = superblock.s_inode_size;		// size of inode structure

    inodeTableOffset = superBlockOffset + (first_inode_block - 1) * block_size;

    struct ext2_inode inode;

    for (__u32 inum = 1; inum <= inodes_count; inum++){
        off_t inode_offset = inodeTableOffset + (inum - 1) * inode_size;
        pread(input_fd,&inode,inode_size,inode_offset);
        __u16 mode = inode.i_mode;
        __u16 links_count = inode.i_links_count;
        if (mode != 0 && links_count != 0) {		// allocated = non-zero mode and non-zero link count
            char file_type;
            if (S_ISREG(mode))    file_type = 'f';
            else if (S_ISDIR(mode)){
                file_type = 'd';
                total_offset = 0;   //offset of each file within the directory
                print_dir(inum,&inode);
            }
            else if (S_ISLNK(mode))    file_type = 's';
            else    file_type = '?';

            __u16 mode_low_12 = mode & 0x0FFF;
            __u16 owner = inode.i_uid;
            __u16 group = inode.i_gid;
            __u32 ctime = inode.i_ctime;
            __u32 mtime = inode.i_mtime;
            __u32 atime = inode.i_atime;
            char formatted_ctime[80];
            char formatted_mtime[80];
            char formatted_atime[80];
            format_time(ctime, formatted_ctime);
            format_time(mtime, formatted_mtime);
            format_time(atime, formatted_atime);

            uint64_t low_file_size = (uint64_t)inode.i_size;
            uint64_t high_file_size = (uint64_t)inode.i_dir_acl;
            uint64_t file_size = (high_file_size << 32) | low_file_size;

            __u32 blocks_count = inode.i_blocks;

            fprintf(stdout,"INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%lu,%u",inum,
                    file_type,mode_low_12,owner,group,links_count,formatted_ctime,
                    formatted_mtime,formatted_atime,file_size,blocks_count);

            for (int i = 0; i < 15; i++) {
                fprintf(stdout,",%u", inode.i_block[i]);
            }
            fprintf(stdout,"\n");


			if (file_type == 'd' || file_type == 'f') {
			    ///////////////////single///////////////////
                if (inode.i_block[12] != 0) {
  					if (pread(input_fd, data_block1, block_size, superBlockOffset + (inode.i_block[12] - 1) * block_size) == -1) {
  						syscall_error();
  					}
  					for (unsigned int i = 0; i != block_size/4; i++) {
  						if (data_block1[i] != 0) {
  							printf("INDIRECT,%d,%d,%d,%d,%d\n",
  									inum,
  									1,
  									singleOffset + i,
  									inode.i_block[12],
  									data_block1[i]);
  						}
  					}
                }

			    /////////////////////double//////////////////////
                if (inode.i_block[13] != 0) {
  					if (pread(input_fd, data_block1, block_size, superBlockOffset + (inode.i_block[13] - 1) * block_size) == -1) {
  						syscall_error();
  					}
  					// loop through first array of pointers
  					for (unsigned int i = 0; i != block_size/4; i++) {
  						if (data_block1[i] != 0) {
  							printf("INDIRECT,%d,%d,%d,%d,%d\n",
  									inum,
  									2,
  									doubleOffset,
  									inode.i_block[13],
  									data_block1[i]);
  						}

  						if (pread(input_fd, data_block2, block_size, superBlockOffset + (data_block1[i] - 1) * block_size) == -1) {
  							syscall_error();
  						}
  						// loop through second set of array of pointers
  						for (unsigned int j = 0; j != block_size/4; j++) {
  							if (data_block2[j] != 0) {
  								printf("INDIRECT,%d,%d,%d,%d,%d\n",
    								inum,
    								1,
    								doubleOffset + (i * 256) + j,
    								data_block1[i],
    								data_block2[j]);
  							}
  						}
  				    }
                }

				////////////////////triple//////////////////////
                if (inode.i_block[14] != 0) {
  					if (pread(input_fd, data_block1, block_size, superBlockOffset + (inode.i_block[14] - 1) * block_size) == -1) {
  						syscall_error();
  					}
  					for (unsigned int i = 0; i != block_size/4; i++) {
  						if (data_block1[i] != 0) {
  							printf("INDIRECT,%d,%d,%d,%d,%d\n",
								inum,
								3,
								tripleOffset,
								inode.i_block[14],
								data_block1[i]);
  						}

  						if (pread(input_fd, data_block2, block_size, superBlockOffset + (data_block1[i] - 1) * block_size) == -1) {
  							syscall_error();
  						}
  						// loop through second set of array of pointers
  						for (unsigned int j = 0; j != block_size/4; j++) {
  							if (data_block2[j] != 0) {
  								printf("INDIRECT,%d,%d,%d,%d,%d\n",
									inum,
									2,
									tripleOffset + (i * 256) + j,
									data_block1[i],
									data_block2[j]);
  							}

  							if (pread(input_fd, data_block3, block_size, superBlockOffset + (data_block2[j] - 1) * block_size) == -1) {
  								syscall_error();
  							}
  							// loop through third set of array of pointers
  							for (unsigned int k = 0; k != block_size/4; k++) {
  								if (data_block3[k] != 0) {
  									printf("INDIRECT,%d,%d,%d,%d,%d\n",
										inum,
										1,
										tripleOffset + (i * 256) + (j * 256) + k,
										data_block2[j],
										data_block3[k]);
  								}
  							}
  						}
  					}
                }
            }
		}
    }
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
    freeBlockEntries();
    freeInodeEntries();
    inode_summary();
}
