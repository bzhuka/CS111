#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SUPERBLOCK_SIZE 1024

//file descriptor of the disk-image
int fd;

//global sizes
uint32_t block_size;
uint32_t frag_size;

//number of groups
int nGroups;

//Directories and stuff
int* pDirectories;
int* pInodeNumber;
int nDirectories = 0;

//valid inodes
int* pInodes;
int nInodes = 0;

//Superblock structure
struct superblock {
	uint32_t s_inodes_count;
	uint32_t s_blocks_count;
	uint32_t s_r_blocks_count;
	uint32_t s_free_blocks_count;
	uint32_t s_free_inodes_count;

	uint32_t s_first_data_block;
	uint32_t s_log_block_size;
	uint32_t s_log_frag_size;
	uint32_t s_blocks_per_group;
	uint32_t s_frags_per_group;

	uint32_t s_inodes_per_group;
	uint32_t s_mtime;
	uint32_t s_wtime;
	uint16_t s_mnt_count;
	uint16_t s_max_mnt_count;
	uint16_t s_magic;
	uint16_t s_state;

	uint16_t s_errors;
	uint16_t s_minor_rev_level;
	uint32_t s_lastcheck;
	uint32_t s_checkinterval;
	uint32_t s_creator_os;
	uint32_t s_rev_level;

	uint16_t s_def_resuid;
	uint16_t s_def_resgid;
	uint8_t DYNAMIC_REV_specific[SUPERBLOCK_SIZE - 84];
} superblock;

//Block group descriptor structure
struct group_descriptor {
	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
	uint16_t bg_pad;
	uint32_t bg_reserved[3];
};

struct group_descriptor *groupDescriptionTable;

//Inode structure
struct inode {
	uint16_t i_mode;
	uint16_t i_uid;
	uint32_t i_size;
	uint32_t i_atime;
	uint32_t i_ctime;
	uint32_t i_mtime;
	uint32_t i_dtime;
	uint16_t i_gid;
	uint16_t i_links_count;
	uint32_t i_blocks;
	uint32_t i_flags;
	uint32_t i_osd1;
	uint32_t i_block[15];
	uint32_t i_generation;
	uint32_t i_file_acl;
	uint32_t i_dir_acl;
	uint32_t i_faddr;
	uint16_t i_osd2[6];
} inode;

//Directory Structure
struct directory {
	uint32_t inode;
	uint16_t rec_len;
	uint8_t name_len;
	uint8_t file_type;
	char name[256];
} directory;

//function to generate csv file for superblock
int fSuperblock() {
	int ofd = creat("super.csv", 0666);
	if (ofd < 0) {
		fprintf(stderr, "couldn't create super.csv");
		exit(-1);
	}

	pread(fd, &superblock, SUPERBLOCK_SIZE, 1024);

	//Generate csv file and sanity checking
	char message[4096] = {};
	int i;

	//Magic number I don't know how to check if magic number is correct
	i = sprintf(message, "%x,", superblock.s_magic);
	write(ofd, message, i);

	//total number of inodes and total number of blocks
	i = sprintf(message, "%"PRIu32",%"PRIu32",", superblock.s_inodes_count, superblock.s_blocks_count);
	write(ofd, message, i);

	//block size
	block_size = 1024 << superblock.s_log_block_size;
	if (!(block_size == 512 || block_size == 1024 || block_size == 2048 || block_size==4096 || block_size == 8192 || block_size == 16384 || block_size == 32768 || block_size == 65536)) {
		fprintf(stderr, "Superblock - invalid block size: %"PRIu32, block_size);
		exit(-1);
	}
	i = sprintf(message, "%"PRIu32",", block_size);
	write(ofd, message, i);

	//frag size
	int32_t shift_value = (int32_t) superblock.s_log_frag_size;
        if (shift_value < 0) {
                frag_size = 1024 >> -shift_value;
        } else {
                frag_size = 1024 << shift_value;
        }
	i = sprintf(message, "%"PRIu32",", frag_size);
	write(ofd, message, i);

	//blocks per group
	if (superblock.s_blocks_count % superblock.s_blocks_per_group != 0) {
		fprintf(stderr, "%"PRIu32" blocks, %"PRIu32" blocks/group", superblock.s_blocks_count, superblock.s_blocks_per_group);
		exit(-1);
	}
	i = sprintf(message, "%"PRIu32",", superblock.s_blocks_per_group);
	write(ofd, message, i);

	//inodes per group
	if (superblock.s_inodes_count % superblock.s_inodes_per_group != 0) {
		fprintf(stderr, "%"PRIu32" inodes, %"PRIu32" inodes/group", superblock.s_inodes_count, superblock.s_inodes_per_group);
		exit(-1);
	}
	i = sprintf(message, "%"PRIu32",", superblock.s_inodes_per_group);
	write(ofd, message, i);

	//fragments per group	
	i = sprintf(message, "%"PRIu32",", superblock.s_frags_per_group);
	write(ofd, message, i);	

	//first data block
	i = sprintf(message, "%"PRIu32"\n", superblock.s_first_data_block);
	write(ofd, message, i);

	close(ofd);
	return 1;
}

//function to generate group.csv
int fGroupDescriptor() {
	int ofd = creat("group.csv", 0666);
        if (ofd < 0) {
                fprintf(stderr, "couldn't create group.csv");
                exit(-1);
        }

	//some needed variables
	nGroups = superblock.s_blocks_count / superblock.s_blocks_per_group;
	
	groupDescriptionTable = malloc(sizeof(struct group_descriptor) * nGroups);
	pread(fd, groupDescriptionTable, sizeof(struct group_descriptor) * nGroups, (superblock.s_first_data_block+1) * block_size);

	char message[4096] = {};
	int i;
	int j;
	int nBlocks = 0;
	for (i = 0; i < nGroups; i++) {
		//Number of contained blocks
		//Special case for last group
		if ( i == nGroups - 1) {
			j = sprintf(message, "%"PRIu32",", (superblock.s_blocks_count - nBlocks));
			write(ofd, message, j);
		} else {
			nBlocks += superblock.s_blocks_per_group;
			j = sprintf(message, "%"PRIu32",", superblock.s_blocks_per_group);
			write(ofd, message, j);
		}

		//Number of free blocks
		j = sprintf(message, "%"PRIu32",", groupDescriptionTable[i].bg_free_blocks_count);
		write(ofd, message, j);

		//Number of free inodes
		j = sprintf(message, "%"PRIu32",", groupDescriptionTable[i].bg_free_inodes_count);
		write(ofd, message, j);

		//Number of directories
		j = sprintf(message, "%"PRIu32",", groupDescriptionTable[i].bg_used_dirs_count);
		write(ofd, message, j);

		//(free) inode bitmap block
		j = sprintf(message, "%x,", groupDescriptionTable[i].bg_inode_bitmap);
		write(ofd, message, j);

		//(free) block bitmap block
		j = sprintf(message, "%x,", groupDescriptionTable[i].bg_block_bitmap);
		write(ofd, message, j);

		//inode table (start) block
		j = sprintf(message, "%x\n", groupDescriptionTable[i].bg_inode_table);
		write(ofd, message, j);
	}
	close(ofd);
	return 1;
}

//Function to list free inodes and free blocks into bitmap.csv
int fFreeBitmapEntry() {
	int ofd = creat("bitmap.csv", 0666);
	if (ofd < 0) {
		fprintf(stderr, "couldn't create bitmap.csv");
		exit(-1);
	}

	uint8_t free_block_bitmap[sizeof(uint8_t) * block_size];
	uint8_t free_inode_bitmap[sizeof(uint8_t) * block_size];

	char message[4096] = {};
	int i;
	int j;
	int k;
	int l;
	int block_counter = 1;
	int inode_counter = 1;
	for (i = 0; i < nGroups; i++) {
		pread(fd, free_block_bitmap, block_size, groupDescriptionTable[i].bg_block_bitmap*block_size); 
		for(j = 0; j < block_size; j++) {
			if(block_counter > superblock.s_blocks_per_group * (i+1))
				break;
			for(k = 0; k < 8; k++) {
				if( ((free_block_bitmap[j] & (1 << k)) >> k) == 0) {//gets each bit
					l = sprintf(message, "%x,%"PRIu32"\n", groupDescriptionTable[i].bg_block_bitmap, block_counter);
					write(ofd, message, l);
				}
				block_counter++;
			}
		}
		pread(fd, free_inode_bitmap, block_size, groupDescriptionTable[i].bg_inode_bitmap*block_size);
		for(j = 0; j < block_size; j++) {
			if (inode_counter > superblock.s_inodes_per_group * (i+1))
				break;
			for(k = 0; k < 8; k++) {
				if( ((free_inode_bitmap[j] & (1 << k)) >> k) == 0) {//gets each bit
					l = sprintf(message, "%x,%"PRIu32"\n", groupDescriptionTable[i].bg_inode_bitmap, inode_counter);
					write(ofd, message, l);
				}
				inode_counter++;
			}
		}
	}
	close(ofd);
	return 1;	
}

//function to create inode.csv
int fInodes() {
	int ofd = creat("inode.csv", 0666);
	if (ofd < 0) {
		fprintf(stderr, "Couldn't create inode.csv correctly");
		exit(-1);
	}

	//set up directories stuff
	pDirectories = malloc(superblock.s_inodes_count * sizeof(int));
	pInodeNumber = malloc(superblock.s_inodes_count * sizeof(int));

	//set up inode array
	pInodes = malloc(superblock.s_inodes_count * sizeof(int));

	char message[4096] = {};
	uint8_t inode_bitmap[sizeof(uint8_t) * block_size];
	int i;
	int j;
	int k;
	int l;
	int m;
	int counter = 1;
	for (i = 0; i < nGroups; i++) {
		pread(fd, inode_bitmap, block_size, groupDescriptionTable[i].bg_inode_bitmap*block_size);
		for (j = 0; j < block_size; j++) {
			if (counter > superblock.s_inodes_per_group * (i+1))
				break;
			for (k=0; k < 8; k++) {
				if( ((inode_bitmap[j] & (1 << k)) >> k) != 0) {
					pread(fd, &inode, sizeof(struct inode), groupDescriptionTable[i].bg_inode_table*block_size + sizeof(struct inode)*(j*8 + k));

					//insert into inode array
					pInodes[nInodes] = groupDescriptionTable[i].bg_inode_table * block_size + sizeof(struct inode)*(j*8 + k);
					nInodes++;

					//Inode number
					l = sprintf(message, "%d,", counter);
					write(ofd, message, l);

					uint16_t mode = inode.i_mode;	
					//file type
					if ((mode & 0x4000) == 0x4000) {//directory
						pDirectories[nDirectories] = groupDescriptionTable[i].bg_inode_table*block_size + sizeof(struct inode)*(j*8 + k);
						pInodeNumber[nDirectories] = counter;
						nDirectories++;
						l = sprintf(message, "d,");
						write(ofd, message, l);
					} else if ((mode & 0xA000) == 0xA000) {//symbolic link
						l = sprintf(message, "s,");
						write(ofd, message, l);
					} else if ((mode & 0x8000) == 0x8000) {//regular file
						l = sprintf(message, "f,");
						write(ofd, message, l);
					} else {//just do ?
						l = sprintf(message, "?,");
						write(ofd, message, l);
					}

					//mode
					l = sprintf(message, "%o,", mode);
					write(ofd, message, l);

					//owner
					l = sprintf(message, "%"PRIu32",", (inode.i_osd2[2] << 16) + inode.i_uid);
					write(ofd, message, l);

					//group
					l = sprintf(message, "%"PRIu16",", (inode.i_osd2[3] << 16) + inode.i_gid);
					write(ofd, message, l);

					//link count
					l = sprintf(message, "%u,", inode.i_links_count);
					write(ofd, message, l);

					//creation time
					l = sprintf(message, "%x,", inode.i_ctime);
					write(ofd, message, l);

					//modification time
					l = sprintf(message, "%x,", inode.i_mtime);
					write(ofd, message, l);

					//access time
					l = sprintf(message, "%x,", inode.i_atime);
					write(ofd, message, l);

					//file size
					l = sprintf(message, "%u,", inode.i_size);
					write(ofd, message, l);

					//number of blocks
					l = sprintf(message, "%u,", (inode.i_blocks/(2<<superblock.s_log_block_size)));
					write(ofd, message, l);

					//15 block pointers
					for (m = 0; m < 14; m++) {
						l = sprintf(message, "%x,", inode.i_block[m]);
						write(ofd, message, l);
					}
					l = sprintf(message, "%x\n", inode.i_block[14]);
					write(ofd, message, l);
				}

				counter++;
				if (counter > superblock.s_inodes_per_group * (i+1))
					break;
			}
		}
	}

	close(ofd);
	return 1;
}

//function to create directory.csv
int fDirectories() {
	int ofd = creat("directory.csv", 0666);
	if (ofd < 0) {
		fprintf(stderr, "couldn't create directory.csv");
		exit(-1);
	}

	char message[4096] = {};
	int i;
	int j;
	int l;
	int counter;
	uint32_t offset;
	for (i=0; i < nDirectories; i++) {
		counter = 0;
		for (j=0; j < 12; j++) {
			pread(fd, &offset, 4, pDirectories[i] + 40 + (j*4));
			if (offset != 0) {
				int address = offset * block_size;
				while(address < offset * block_size + block_size) {
					pread(fd, &directory, 8, address);
					pread(fd, &(directory.name), directory.name_len, address+8);

					if(directory.inode == 0) {
						address += directory.rec_len;
						counter++;
					} else {

						//parent inode number
						l = sprintf(message, "%u,", pInodeNumber[i]);
						write(ofd, message, l);

						//entry number
						l = sprintf(message, "%u,", counter);
						write(ofd, message, l);

						//entry length
						l = sprintf(message, "%u,", directory.rec_len);
						write(ofd, message, l);

						//name length
						l = sprintf(message, "%u,", directory.name_len);
						write(ofd, message, l);

						//inode number of the file entry
						l = sprintf(message, "%u,", directory.inode);
						write(ofd, message, l);

						//name
						l = sprintf(message, "\"%s", directory.name);
						write(ofd, message, directory.name_len+1);
						l = sprintf(message, "\"\n");
						write(ofd, message, l);

						counter++;
						address+=directory.rec_len;
					}
				}
			}

		}

		//Indirect blocks
		uint32_t initial_address;
		pread(fd, &initial_address, 4, pDirectories[i] + 88);
		if(initial_address != 0) {
			for (j=0; j < block_size / 4; j++) {
				int address = block_size * initial_address + (j*4);
				pread(fd, &offset, 4, address);
				if (offset != 0) {
					address = offset * block_size;
					while(address < offset * block_size + block_size) {
						pread(fd, &directory, 8, address);
						pread(fd, &(directory.name), directory.name_len, address+8);

						if(directory.inode == 0) {
							address += directory.rec_len;
							counter++;
						} else {
							//parent inode number
							l = sprintf(message, "%u,", pInodeNumber[i]);
							write(ofd, message, l);

							//entry number
							l = sprintf(message, "%u,", counter);
							write(ofd, message, l);

							//entry length
							l = sprintf(message, "%u,", directory.rec_len);
							write(ofd, message, l);

							//name length
							l = sprintf(message, "%u,", directory.name_len);
							write(ofd, message, l);

							//inode number of the file entry
							l = sprintf(message, "%u,", directory.inode);
							write(ofd, message, l);

							//name
							l = sprintf(message, "\"%s", directory.name);
							write(ofd, message, directory.name_len+1);
							l = sprintf(message, "\"\n");
							write(ofd, message, l);

							counter++;
							address+=directory.rec_len;
						}
					}
				}
			}
		}	
	}

	close(ofd);
	return 1;
}

//function to help produce indirect.csv
int fHelpIndirect(int ofd, uint32_t block_address) {
	int j;
	int l;
	int counter = 0;
	uint32_t pointer;
	char message[4096] = {};
	for (j = 0; j < block_size / 4; j++) {
		pread(fd, &pointer, 4, block_address * block_size + (j*4));
		if (pointer != 0) {	
			//block number of containing block
			l = sprintf(message, "%x,", block_address);
			write(ofd, message, l);

			//entry number within that block
			l = sprintf(message, "%u,", counter);
			write(ofd, message, l);

			//block pointer value
			l = sprintf(message, "%x\n", pointer);
			write(ofd, message, l);

			counter++;
		}
	}
}

//function to produce indirect.csv
int fIndirectBlock() {
	int ofd = creat("indirect.csv", 0666);
	if (ofd < 0) {
		fprintf(stderr, "couldn't create indirect.csv");
		exit(-1);
	}

	uint32_t block_address;
	uint32_t block_address2;
	uint32_t block_address3;
	uint32_t pointer;
	int i;
	int j;
	int k;
	for (i = 0; i < nInodes; i++) {
		//single
		pread(fd, &block_address, 4, pInodes[i] + 88);
		fHelpIndirect(ofd, block_address);
/*
		//double
		pread(fd, &block_address, 4, pInodes[i] + 92);
		fHelpIndirect(ofd, block_address);
		for (j = 0; j < block_size / 4; j++) {
			pread(fd, &block_address2, 4, block_address * block_size + (j*4));
			if (block_address2 !=0) {
				fHelpIndirect(ofd, block_address2);
			}
		}
	
		//triple
		pread(fd, &block_address, 4, pInodes[i] + 96);
		fHelpIndirect(ofd, block_address);
		for (j = 0; j < block_size / 4; j++) {
			pread(fd, &block_address2, 4, block_address * block_size + (j*4));
			if (block_address2 !=0) {
				fHelpIndirect(ofd, block_address2);
			}
		}
		for (j = 0; j < block_size / 4; j++) {
			pread(fd, &block_address2, 4, block_address * block_size + (j*4));
			if (block_address2 !=0) {
				for (k = 0; k < block_size / 4; k++) {
					pread(fd, &block_address3, 4, block_address2 * block_size + (k*4));
					if (block_address3 != 0) {
						fHelpIndirect(ofd, block_address3);
					}	
				}
			}
		}*/
	}
	close(ofd);
}

int main(int argc, char*argv[]) {
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Didn't open image correctly");
		exit(-1);
	}

	fSuperblock();
	fGroupDescriptor();
	fFreeBitmapEntry();
	fInodes();
	fDirectories();
	fIndirectBlock();
}
