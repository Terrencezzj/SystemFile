#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <errno.h>
#include "ext2.h"
#include "ext2_util.h"

#define USAGE "Usage: %s <image file name> <absolute restore path>\n"


/*
 * Check if the input entry is valid entry
 */
unsigned char is_entry(struct ext2_dir_entry *entry){
  	if ((entry->inode <= 11 &&  entry->inode > 2) ||
  		(entry->inode > NUM_INODES) ||
  		(entry->rec_len > EXT2_BLOCK_SIZE) ||
  		(entry->name_len > EXT2_NAME_LEN) ||
  		(entry->rec_len < 0) ||
  		(entry->file_type > EXT2_FT_MAX)
  		){
    	return 1;
  	}
  	return 0;
}


/*
 * Find deleted entry in blocks of parent directory
 * If not found, exit with error
 */
struct ext2_dir_entry *find_deleted_entry(struct ext2_dir_entry *parent_entry,
	struct ext2_dir_entry **prev_entry_ptr, int *rec_len_ptr,
	char *filename){
	struct ext2_dir_entry *cur_dir;
	struct ext2_dir_entry *small_entry;
	int ent_size;
	int small_ent_size;
	// Inode for the parent direcotry
	struct ext2_inode *parent_inode = get_inode_by_num(parent_entry->inode);
	// The expected entry size of the target file/link
	int file_size = entry_size(strlen(filename));

	// Loop through all data blocks
	for (int i = 0; i < parent_inode->i_blocks / 2; i++){
		int total_size = 0;
		int rec_len = 0;
		if (i < 13){
			cur_dir = (struct ext2_dir_entry *) (disk + dir_inode->i_block[i] * EXT2_BLOCK_SIZE);
		}
		// the i_block is contained in the indirect pointer
		else{
			// break;
			unsigned int block_index = ((unsigned int *)
				(disk + inode->i_block[12] * EXT2_BLOCK_SIZE))[i - 13];
			cur_dir = (struct ext2_dir_entry *) (disk + block_index * EXT2_BLOCK_SIZE);
		}
		if (strncmp(cur_dir->name, filename, cur_dir->name_len) == 0){
			perror("UNRECOVERBLE");
			exit(ENOENT);
		}
		// Loop through all the blocks
		while (total_size < EXT2_BLOCK_SIZE){
			ent_size = entry_size(strlen(cur_dir->name_len));
			*rec_len_ptr = ent_size;
			// Loop through small gap in block
			while (ent_size + file_size < cur_dir->rec_len) {
				small_entry = (void *)cur_dir + ent_size;
				if (is_entry(next_dir)) {
					char *file_name = malloc(small_entry->name_len + 1);
					file_name = strncpy(file_name, small_entry->name, small_entry->name_len);
					file_name[next_dir->name_len] = '\0';
					// find the correspond directroy
					if(strcmp(file_name, filename) == 0){
						if(small_entry->file_type == EXT2_FT_REG_FILE){
							*prev_entry_ptr = cur_dir;
							return small_entry;
						}
						// EISDIR if not file
						free(file_name);
						perror("RESTORE PATH IS DIR");
						exit(EISDIR);
					}
					free(file_name);
				}
				small_ent_size = entry_size(strlen(small_entry->name_len));
				*rec_len_ptr += small_ent_size;
				ent_size += small_ent_size;
			}
			cur_dir = (void *)cur_dir + rec_len;
			rec_len = cur_dir->rec_len;
			total_size += rec_len;						 						
		}
	perror("RESTORE ENTRY NOT FOUND");
	exit(ENOENT);
	return NULL;
}


/*
 * Restore the inode and all data blocks of the given entry.
 */
void restore(struct ext2_dir_entry *entry, struct ext2_dir_entry *prev_entry, int rec_len){
	// Unrecoverable if the inode has been reused.
  	if (get_inode_bitmap(entry->inode)){
		perror("UNRECOVERBLE");
		exit(ENOENT);
	}
  	struct ext2_inode *inode = get_inode_by_num(entry->inode);
  	// Loop through for the first time to check every data block
  	for (int i = 0; i < inode->i_blocks / 2; i++){
		unsigned int block_index;
		if (i < 13){
	  		block_index = inode->i_block[i];
		} else {
	  		unsigned int *indirect_block = (unsigned int *)
	  			(disk + inode->i_block[12] * EXT2_BLOCK_SIZE);
	  		block_index = indirect_block[i - 13];
		}
		// Unrecoverable if any of the data blocks has been reused.
		if (get_block_bitmap(block_index)){
			perror("UNRECOVERBLE");
			exit(ENOENT);
		}
  	}
  	// Check finished, Restore inode
  	// Loop through for the second time to restore every data blocks
  	for (int i = 0; i < inode->i_blocks/2; i++){
		unsigned int block_index;
		if (i < 13){
	  		block_index = inode->i_block[i];
		} else {
	  		unsigned int *indirect_block = (unsigned int *)
	  			(disk + inode->i_block[12] * EXT2_BLOCK_SIZE);
	  		block_index = indirect_block[i - 13];
		}
		set_block_bitmap(block_index, 1);
		sb->s_free_blocks_count--;
		gd->bg_free_blocks_count--;
  	}
  	// Restore inode
  	set_inode_bitmap(entry->inode, 1);
  	sb->s_free_inodes_count--;
  	gd->bg_free_inodes_count--;
  	inode->i_dtime = 0;
  	inode->i_links_count++;
  	// Restore entry
  	entry->rec_len = prev_entry->rec_len - rec_len;
  	prev_entry->rec_len = rec_len;
}



int main(int argc, char **argv) {
	// Variables
	unsigned int parent_inode_num;
	unsigned int restore_parent_index;
	unsigned int dir_block_num;
	int rec_len;
	struct ext2_dir_entry *restore_entry;
	struct ext2_dir_entry *prev_entry;
	struct ext2_inode *parent_inode;
	struct ext2_dir_entry *parent_entry;
	char *restore_path;
	char *restore_parent;
	char *restore_filename;	


	// Initialization
	if (argc != 3) {
		fprintf(stderr, USAGE, argv[0]);
		exit(EINVAL);
	}

	// Check path
	if (is_not_absolute(argv[2])) {
		perror("NOT ABSOLUTE PATH");
		exit(EINVAL);
	}

	restore_path = argv[2]
	if (restore_path[strlen(restore_path) - 1] == '/') {
		perror("RESTORE PATH IS DIR");
		exit(EISDIR);
	}

	disk_image_path = argv[1];
	mount_ex2(disk_image_path);


	char path_copy[strlen(restore_path) + 1];
	strncpy(path_copy, restore_path, strlen(restore_path));
	path_copy[strlen(restore_path)] = '\0';
	restore_parent = dirname(path_copy);
	restore_filename = basename(restore_path);

	restore_parent_index = get_index(restore_parent);
	parent_inode = get_inode_by_num(restore_parent_index);
	parent_entry = find_parent_dir(restore_parent);

	// Check if parent is directory
	if(!parent_inode->i_mode & EXT2_S_IFDIR){
		perror("RESTORE PATH PARENT IS NOT DIR");
		exit(ENOENT);
	}

	// Check if the file to restore exists (Not removed)
	if (check_file(parent_inode, restore_filename) != NULL) {
		exit(EEXIST);
	}

	restore_entry = find_deleted_entry(parent_entry, &prev_entry, &rec_len, restore_filename);

	// Restore the entry.
	restore(restore_entry, prev_entry, rec_len);

}
