#include "ext4_structures.h"

/* read volume label from super block sb and write it to stdout */
void printLabel(struct ext4_super_block *sb);

/* given a path, print the contents of the directory
 *
 * path: absolute path to a directory
 * fd: file descriptor to the EXT4_IMAGE_FILE
 * sb: reference to the super block
 * gd: reference to the first group descriptor
 */
void printDirectoryByPath(char *path, int fd, struct ext4_super_block *sb, struct ext4_group_desc *gd);

/* given a path, print the contents of the file
 *
 * path: absolute path to a file
 * fd: file descriptor to the EXT4_IMAGE_FILE
 * sb: reference to the super block
 * gd: reference to the first group descriptor
 */
void printFileByPath(char *path, int fd, struct ext4_super_block *sb, struct ext4_group_desc *gd);

/* 
 * retrieve I-Node by I-Node number
 *
 * fd: file descriptor to the EXT4_IMAGE_FILE
 * inode: reference to a struct ext4_inode, where the I-Node should be stored
 * inode_no: number of the I-Node to be retrieved
 * sb: reference to the super block
 * gd: reference to the first group descriptor
 */
void getInode(int fd, struct ext4_inode *inode, int inode_no, struct ext4_super_block *sb, struct ext4_group_desc *gd);

/*
 * retrieve data blocks of I-Node
 *
 * inode: reference to a struct ext4_inode, from which the data blocks should be extracted
 * blocks: reference to an array of int, where the block numbers should be stored
 * nblocks: reference to an int, where the number of data blocks should be stored
 */
void getDataBlocks(struct ext4_inode *inode, int *blocks, int *nblocks);

/*
 * print the contents of a directory
 *
 * fd: file descriptor to the EXT4_IMAGE_FILE
 * blocks: reference to the data blocks of the directory
 * nblocks: number of blocks in parameter blocks
 */
void printDir(int fd, int *blocks, int nblocks);

/*
 * print the contents of a file
 *
 * fd: file descriptor to the EXT4_IMAGE_FILE
 * blocks: reference to the data blocks of the file
 * nblocks: number of blocks in parameter blocks
 */
void printFile(int fd, int *blocks, int nblocks);

/*
 * get the I-Node number for a path
 *
 * path: absolute path to a directory or file
 * fd: file descriptor to the EXT4_IMAGE_FILE
 * sb: reference to the super block
 * gd: reference to the first group descriptor
 *
 * returns corresponding I-Node number of -1 on failure
 */
int getInodeFromPath(char *path, int fd, struct ext4_super_block *sb, struct ext4_group_desc *gd);

/*
 * get the I-Node number of target
 * 
 * fd: file descriptor to the EXT4_IMAGE_FILE
 * blocks: reference to the data blocks of the directory to search in
 * nblocks: number of blocks in parameter blocks
 * target: what file or directory to look for
 *
 * return corresponding I-Node number or -1 on failure
 */
int searchDir(int fd, int *blocks, int nblocks, char *target);
