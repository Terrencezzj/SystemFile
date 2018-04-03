#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_util.h"

#define USAGE "Usage: %s <image file name>\n"
int fixes_count;

/* ----------------------- Helper functions ------------------------*/
/*
 * Output of a.
 */
void fix_output(char* block, char* bitmap, int real, int fake) {
	int difference = 0;
	difference = abs(real - fake);

	if (difference > 0) {
		printf("Fixed: %s's free %s counter was off by %u compared to the bitmap\n",
		block, bitmap, difference);
		fixes_count += difference;
	}
}


/*
 * A.Fix the inconsistencies between the bitmaps and the free blocks, free inodes counters.
 */
void fix_bitmaps() {
	// Variables
	int i;
	int j;
	int free_inode_count = 0;
	int free_block_count = 0;

		// Count actual free blocks and inodes
	for(i = 0; i < sb->s_inodes_count / BYTE_LENGTH; i++){
		for(j = 0; j < BYTE_LENGTH; j++){
			if ((inode_bitmap[i] & (1 << j)) == 0){
				free_inode_count ++;
			}
		}
	}
	for(i = 0; i < sb->s_blocks_count / BYTE_LENGTH; i++){
		for(j = 0; j < BYTE_LENGTH; j++){
			if ((block_bitmap[i] & (1 << j)) == 0){
				free_block_count ++;
			}
		}
	}

	// Check if exists inconsistencies
	if (free_inode_count != gd->bg_free_inodes_count) { 
		fix_output("block group", "inodes", free_inode_count, gd->bg_free_inodes_count);
		gd->bg_free_inodes_count = free_inode_count;
	}
	if (free_inode_count != sb->s_free_inodes_count) { 
		fix_output("superblock", "inodes", free_inode_count, sb->s_free_inodes_count);
		sb->s_free_inodes_count = free_inode_count;
	}
	if (free_block_count != gd->bg_free_blocks_count) { 
		fix_output("block group", "blocks", free_block_count, gd->bg_free_blocks_count);
		gd->bg_free_blocks_count = free_block_count;
	}
	if (free_block_count != sb->s_free_blocks_count) { 
		fix_output("superblock", "blocks", free_block_count, sb->s_free_blocks_count);
		sb->s_free_blocks_count = free_block_count;
	}
}


/*
 * B.Fix the inconsistencies between the imode and file_type.
 */
void fix_file_type(struct ext2_dir_entry *dir_entry) {
	struct ext2_inode *inode = get_inode_by_num(dir_entry->inode);
	// if not match, fix it.
	if (compare_mode_type(dir_entry, inode) == 0){
		if (inode->i_mode & EXT2_S_IFDIR){
			dir_entry->file_type = EXT2_FT_DIR;
		} else if (inode->i_mode & EXT2_S_IFREG) {
			dir_entry->file_type = EXT2_FT_REG_FILE;
		} else if (inode->i_mode & EXT2_S_IFLNK) {
			dir_entry->file_type = EXT2_FT_SYMLINK;
		}
		printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", dir_entry->inode);
		fixes_count ++;
	}
}


/*
 * C.Check if the inode is marked as used in the bitmap.
 * If not, fix it.
 */
void fix_inode_inuse(unsigned int index){
	if (get_inode_bitmap(index) == 0){
		sb->s_free_inodes_count --;
		gd->bg_free_inodes_count --;
		set_inode_bitmap(index, 1);
		printf("Fixed: inode [%d] not marked as in-use\n", index);
		fixes_count ++;
	}
}

/*
 * D.Fix the inode's i_dtime to 0 if it was not.
 */
int fix_dtime_zero(unsigned int index){
	struct ext2_inode *inode = get_inode_by_num(index);
	if (inode->i_dtime != 0){
		inode->i_dtime = 0;
		printf("Fixed: valid inode marked for deletion: [%d]\n", index);
		fixes_count ++;
	}
}


/*
 * E.Check that its data blocks are allocated in the data bitmap. 
 * If any of its blocks is not allocated, fix this by updating the data bitmap.
 */
void fix_blocks_allocated(unsigned int index){
		int i;
		int fixed_i;
		int local_fixed_count = 0;
		struct ext2_inode *inode = get_inode_by_num(index);
		for (i = 0; i < inode->i_blocks / 2; i++){
			// for the direct blocks
			if (i < 12) {
				fixed_i = inode->i_block[i];
			// for indirect blocks
			} else {
				// break;
				fixed_i = ((unsigned int *)(disk + inode->i_block[12] * EXT2_BLOCK_SIZE))[i - 12];
			}
			// Check if the data block are allocated in the data bitmap.
			if (get_block_bitmap(fixed_i) == 0){
				set_block_bitmap(fixed_i, 1);
				sb->s_free_blocks_count --;
				gd->bg_free_blocks_count --;
				local_fixed_count ++;
				fixes_count ++;
			}
		}

		if (local_fixed_count > 0) {
			printf(
				"Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n",
				local_fixed_count, index);
		}
}


/*
 * Recursively do the fix
 */
void fix_looper(struct ext2_dir_entry *dir_entry) {
	int i;
	struct ext2_inode *inode = get_inode_by_num(dir_entry->inode);
	fix_inode_inuse(dir_entry->inode);
	fix_dtime_zero(dir_entry->inode);
	fix_blocks_allocated(dir_entry->inode);
	// If dir_entry is not a directory, return
	if (dir_entry->file_type != EXT2_FT_DIR) {
		return;
	}
	fix_file_type(dir_entry);
	for (i = 0; i < inode->i_blocks / 2; i++) {
							int total_size = 0;
		int rec_len = 0;
		struct ext2_dir_entry *cur_entry = (struct ext2_dir_entry *) 
				(disk + EXT2_BLOCK_SIZE * inode->i_block[i]);
		while (total_size < EXT2_BLOCK_SIZE) {
			// Check cur_entry's file type.
			if (cur_entry->inode != 0) {
				fix_file_type(cur_entry);
			}
			if (cur_entry->name[0] != '.' && cur_entry->inode != 0){
				fix_looper(cur_entry);
			}
			rec_len = cur_entry->rec_len;
			cur_entry = (void *)cur_entry + rec_len;
			total_size += rec_len;
		}
	}
}

/* ---------------------- Helper functions end----------------------*/

/*
 * Main
 */
int main(int argc, char **argv) {
	// Variables
	char *disk_image_path;
	// Initialization
	if (argc != 2) {
		fprintf(stderr, USAGE, argv[0]);
		exit(EINVAL);
	}

	disk_image_path = argv[1];
	mount_ex2(disk_image_path);

	fixes_count = 0;

	// Do the check

	fix_bitmaps();

	struct ext2_inode *root_inode = get_inode_by_num(EXT2_ROOT_INO);
	struct ext2_dir_entry *root_entry = (struct ext2_dir_entry *)
		(disk + root_inode->i_block[0] * EXT2_BLOCK_SIZE);
	fix_looper(root_entry);
	if (fixes_count > 0) {
		printf("%d file system inconsistencies repaired!\n", fixes_count);
	} else {
		printf("No file system inconsistencies detected!\n");
	}
		
}
