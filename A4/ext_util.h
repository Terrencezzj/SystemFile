#include <string.h>
#include "ext2.h"

/* ----------------------- Defined parameters ------------------------*/
#define ENTRY_SIZE_EXCEPT_NAME sizeof(unsigned int) + sizeof(unsigned short)\
 + sizeof(unsigned char) * 2

#define BYTE_LENGTH 8
#define NUM_BLOCKS 128
#define NUM_INODES 32
/* ----------------------- Global Variables ------------------------*/
// The disk
unsigned char *disk;
// The super block
struct ext2_super_block *sb;
// The blocks group descriptor
struct ext2_group_desc *gd;
// The inode table
struct ext2_inode *inode_table;
// The block bitmap
unsigned char *block_bitmap;
// The inode bitmap
unsigned char *inode_bitmap;
/* ----------------------- Utility functions ------------------------*/
struct ext2_dir_entry *get_entry(struct ext2_inode *dir_inode, char * name);
struct ext2_dir_entry *initialize_new_dir(struct ext2_dir_entry *parent, char *name);
int mount_ex2(char *disk_path);
unsigned int *allocate_block(int block_num);
unsigned int allocate_inode();
void set_inode_bitmap(unsigned int new_inode_index, int num);
void set_block_bitmap(unsigned int index, int num);
int set_inode(unsigned int new_inode_index, unsigned short filetype, unsigned int size, \
		unsigned short i_links_count, unsigned int i_blocks_num, \
		unsigned int *i_block);
unsigned short entry_size(name_len);
bool is_not_absolute(char *path);
int get_index(char *path);