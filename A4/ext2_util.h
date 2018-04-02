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
void *inode_table;
// The block bitmap
unsigned char *block_bitmap;
// The inode bitmap
unsigned char *inode_bitmap;
int fixes_count;
/* ----------------------- Utility functions ------------------------*/
struct ext2_dir_entry *get_entry(struct ext2_inode *dir_inode, char * name);
int initialize_new_dir(struct ext2_dir_entry *parent, char *name);
int mount_ex2(char *disk_path);
unsigned int *allocate_block(int block_num);
unsigned int allocate_inode();
void set_inode_bitmap(unsigned int new_inode_index, int num);
void set_block_bitmap(unsigned int index, int num);
int set_inode(unsigned int new_inode_index, unsigned short filetype, unsigned int size, \
		unsigned short i_links_count, unsigned int i_blocks_num, \
		unsigned int *i_block);
unsigned short entry_size(unsigned char name_len);
int add_entry_to_dir(struct ext2_dir_entry *parent_dir, struct ext2_dir_entry *new_dir);
struct ext2_inode *get_inode_by_num(unsigned int num);
struct ext2_dir_entry * find_parent_dir(char *path);
struct ext2_dir_entry *check_file(struct ext2_inode *dir_inode, char * name);
int is_not_absolute(char *path);
int get_index(char *path);
void fix_bitmaps();
unsigned int report_inconsistency(char* field, char* bitmap, int expected, int actual);