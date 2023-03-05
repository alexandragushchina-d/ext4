// io
#include <stdio.h>
#include <stdlib.h>

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// lseek
#include <unistd.h>

// string
#include <string.h>

// errno
#include <errno.h>

// ext4 structures
#include "ext4.h"

#define GROUP_0_PADDING 1024
#define ROOT_INODE 2
#define FILE_TYPE 1
#define DIR_TYPE 2

int g_blockSize = 0;

int getBlockOffset(int blockNo) {
    return g_blockSize * blockNo;
}

int readFile(int fd, void *buf, int count, int offset) {
  lseek(fd, offset, SEEK_SET);
  if(read(fd, buf, count) == -1) {
    return -1;
  }
  return count;
}

char* readBlock(int fd, int blockNo) {
  char* block = (char *) malloc(g_blockSize);
 if (readFile(fd, (void*) block, g_blockSize, getBlockOffset(blockNo)) == -1) {
    fprintf(stderr, "Cannot read block - %d\n", blockNo);
    free(block);
    return 0;
  }

  return block;
}

void printDir(int fd, int *blocks, int nblocks) {
  struct ext4_dir_entry_2* entry;
  char* block = 0;
  for(int i = 0; i < nblocks; ++i) {
    block = readBlock(fd, blocks[i]);
    if (block == 0) {
      continue;
    }

    entry = (struct ext4_dir_entry_2*) block;
    for(;(((char*) entry) - block) < g_blockSize;) {
      if (entry->file_type == 0 || entry->rec_len == 0) {
        break;
      }

      if (entry->file_type != FILE_TYPE && entry->file_type != DIR_TYPE) {
        entry = (struct ext4_dir_entry_2*) (((char*) entry) + entry->rec_len);
        continue;
      }

      if (entry->file_type == DIR_TYPE) {
        printf("Dir : %.*s\n", entry->name_len, entry->name);
      } else {
        printf("File: %.*s\n", entry->name_len, entry->name);
      }

      entry = (struct ext4_dir_entry_2*) (((char*) entry) + entry->rec_len);
    }

    free(block);
  }
}

void printFile(int fd, int *blocks, int nblocks) {
  char* block = 0;
  for(int i = 0; i < nblocks; ++i) {
    block = readBlock(fd, blocks[i]);
    if (block == 0) {
      continue;
    }

    for(int j = 0; block[j] != 0 && j < g_blockSize; ++j) {
      printf("%c", block[j]);
    }

    free(block);
  }
}

void getDataBlocks(struct ext4_inode *inode, int *blocks, int *nblocks) {
  struct ext4_extent_header *header = (struct ext4_extent_header *) inode->i_block;
  struct ext4_extent *extent = 0;
  int blockCount = 0;
  for(int i = 0; i < header->eh_entries; ++i) {
    extent = (struct ext4_extent *) (((char *) header) + sizeof(struct ext4_extent_header) + i * sizeof(struct ext4_extent));
    if (extent->ee_start_hi != 0) {
        fprintf(stderr, "extent->ee_start_hi - %d\n", extent->ee_start_hi);
    }

    for(int j = 0; j < extent->ee_len; ++j) {
      blocks[blockCount++] = extent->ee_start_lo + j;
    }
  }

  *nblocks = blockCount;
}

void getInode(int fd, struct ext4_inode *inode, int inode_no, struct ext4_super_block *sb, struct ext4_group_desc *gd) {
  if (readFile(fd, inode, sizeof(struct ext4_inode), getBlockOffset(gd->bg_inode_table_lo) +  (inode_no - 1) * sb->s_inode_size) == -1) {
    memset(inode, 0, sizeof(struct ext4_inode));
    fprintf(stderr, "Cannot read inode - %d\n", inode_no);
  }
}

int getInodeFromPath(char *path, int fd, struct ext4_super_block *sb, struct ext4_group_desc *gd) {
  char dirName[EXT4_NAME_LEN];
  struct ext4_inode inode;
  char *nextDir = path;
  int blocks[EXT4_N_BLOCKS];
  int nblocks;
  int inodeNo = -1;
  getInode(fd, &inode, ROOT_INODE, sb, gd);
  for (;;) {
    memset(dirName, 0, sizeof(dirName));
    if (nextDir[0] != '/') {
      break;
    }

    ++nextDir;
    while (nextDir[0] != '/' && nextDir[0] != 0) {
      dirName[strlen(dirName)] = nextDir[0];
      ++nextDir;
    }

    if (strlen(dirName) == 0) {
      if (inodeNo == -1) {
        inodeNo = ROOT_INODE;
      }

      break;
    }

    getDataBlocks(&inode, blocks, &nblocks);
    inodeNo = searchDir(fd, blocks, nblocks, dirName);
    if (inodeNo == -1) {
      break;
    }

    getInode(fd, &inode, inodeNo, sb, gd);
    if ((inode.i_mode & 0x4000) == 0 && strlen(nextDir) != 0) {
      return -1;
    }
  }
  return inodeNo;
}

int searchDir(int fd, int *blocks, int nblocks, char *target) {
  struct ext4_dir_entry_2* entry;
  char* block = 0;
  for(int i = 0; i < nblocks; ++i) {
    block = readBlock(fd, blocks[i]);
    if (block == 0) {
      continue;
    }

    entry = (struct ext4_dir_entry_2*) block;
    for(;(((char*) entry) - block) < g_blockSize;) {
      if (entry->file_type == 0 || entry->rec_len == 0) {
        break;
      }

      if (entry->file_type != FILE_TYPE && entry->file_type != DIR_TYPE) {
        entry = (struct ext4_dir_entry_2*) (((char*) entry) + entry->rec_len);
        continue;
      }

      fprintf(stderr, "Entry has been found - '%.*s'\n", entry->name_len, entry->name);
      if (entry->name_len == strlen(target) && strncmp(target, entry->name, entry->name_len) == 0) {
        int result = entry->inode;
        free(block);
        return result;
      }

      entry = (struct ext4_dir_entry_2*) (((char*) entry) + entry->rec_len);
    }

    free(block);
  }
  return -1;
}

void printFileByPath(char *path, int fd, struct ext4_super_block *sb, struct ext4_group_desc *gd) {
  int inodeNo = getInodeFromPath (path, fd, sb, gd);
  if (inodeNo == -1) {
    fprintf(stderr, "Cannot find file by path - %s\n", path);
    return;
  }

  struct ext4_inode inode;
  getInode(fd, &inode, inodeNo, sb, gd);
  int blocks[EXT4_N_BLOCKS];
  int nblocks;
  getDataBlocks (&inode, blocks, &nblocks);
  printFile(fd, blocks, nblocks);
  printf("\n");
}

void printDirectoryByPath(char *path, int fd, struct ext4_super_block *sb, struct ext4_group_desc *gd) {
  int inodeNo = getInodeFromPath (path, fd, sb, gd);
  if (inodeNo == -1) {
    fprintf(stderr, "Cannot find directory by path - %s\n", path);
    return;
  }

  struct ext4_inode inode;
  getInode(fd, &inode, inodeNo, sb, gd);
  int blocks[EXT4_N_BLOCKS];
  int nblocks;
  getDataBlocks (&inode, blocks, &nblocks);
  printDir(fd, blocks, nblocks);
  printf("\n");
}

void printLabel(struct ext4_super_block *sb) {
  printf("The Label of the given ext4 file system is: \"%s\"\n\n", sb->s_volume_name);
}

void print_help(char *prog_name) {
  printf("Usage: %s EXT4_IMAGE_FILE\n", prog_name);
  fflush(stdout);
}

void print_menu() {
  puts("Type your command, then [enter]");
  puts("Available commands:");
  puts("l           Print label of EXT4_IMAGE_FILE");
  puts("d DIRECTORY Print contents of DIRECTORY");
  puts("f FILE      Print contents of FILE");
  puts("q           Quit the program");
  fflush(stdout);
}

int main(int argc, char *argv[]) {
  /* info : https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout */
  /* open fd to ext4 image */
  if (argc != 2) {
    print_help(argv[0]);
    return EXIT_FAILURE;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd == 0) {
    fprintf(stderr, "Cannot open img file - %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  /* extract super block */
  struct ext4_super_block sb;
  if (readFile(fd, &sb, sizeof(struct ext4_super_block), GROUP_0_PADDING) == -1) {
    fprintf(stderr, "Cannot read super block from file - %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  g_blockSize = 1 << (sb.s_log_block_size + 10);
  fprintf(stderr, "Block size - %d\n", g_blockSize);

  /* extract first block group descriptor table */
  int offsetGD = g_blockSize;
  if (g_blockSize == 1024) {
    offsetGD += GROUP_0_PADDING;
  }

  struct ext4_group_desc gd;
  if (readFile(fd, &gd, sizeof(struct ext4_group_desc), offsetGD) == -1) {
    fprintf(stderr, "Cannot read group descriptor from file - %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  char command = ' ';
  char path[1024];
  do {
    print_menu();

    do {
      command = (char) fgetc(stdin);
    } while(command == ' ' || command == '\n');

    switch (command) {
    case 'l':
      printLabel(&sb);
      break;
    case 'd':
      scanf("%s", path);
      fprintf(stderr, "Try to print directory - '%s'\n", path);
      printDirectoryByPath(path, fd, &sb, &gd);
      break;
    case 'f':
      scanf("%s", path);
      fprintf(stderr, "Try to print file - '%s'\n", path);
      printFileByPath(path, fd, &sb, &gd);
    case 'q':
    default:
      break;
    }
  } while(command != 'q');

  return EXIT_SUCCESS;
}
