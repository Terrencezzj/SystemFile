#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include "ext2.h"
#include "ext2_util.h"

#define USAGE "Usage: %s <image file name> [-s] <source path> <target path>\n"


int main(int argc, char *argv[]) {
	// Variables
	char *disk_image_path;
	char *source_path;
	char *target_path;
	char *target_dir_path;
	char *target_filename;
	bool flag = false;
	int src_index;
	int tgt_dir_index;
	int link_index;
	int blocks_num;
	int block_index;
	struct ext2_inode *src;
	struct ext2_inode *tgt_dir_inode;
	struct ext2_inode *link_inode;
	struct ext2_dir_entry *tgt_dir_entry;
	struct ext2_dir_entry *link_entry = malloc(sizeof(struct ext2_dir_entry));

	// Initialization
	// Check input
	if (argc != 4) {
		// Check flag [-s]
		if (argc != 5) {
			fprintf(stderr, USAGE, argv[0]);
			exit(EINVAL);
		} else {
			if (strcmp("-s", argv[2]) != 0) {
				fprintf(stderr, USAGE, argv[0]);
				exit(EINVAL);
			}
			flag = true;
			source_path = argv[3];
			target_path = argv[4];
		}
	} else {
		source_path = argv[2];
		target_path = argv[3];
	}

	// Check path
	if (is_not_absolute(source_path) || is_not_absolute(target_path)) {
		perror("NOT ABSOLUTE PATH");
		exit(EINVAL);
	}

	disk_image_path = argv[1];
	mount_ex2(disk_image_path);


	// 

	src_index = get_index(source_path);
	//tgt_dir_index = get_dir_index(target_path);
	src = get_inode_by_num(src_index);
	//tgt_dir = inode_table + sizeof(struct ext2_inode) * (tgt_dir_index - 1);
	if (!flag && (src->i_mode & EXT2_S_IFDIR)) {
		perror("HARDLINK TO DIR");
		exit(EISDIR);
	}

	target_dir_path = dirname(target_path);
	target_filename = basename(target_path);
	tgt_dir_index = get_index(target_dir_path);
	tgt_dir_inode = get_inode_by_num(target_dir_index);
	tgt_dir_entry = find_parent_dir(target_dir_path);
	// Check if parent is directory
	if(!tgt_dir_inode->i_mode & EXT2_S_IFDIR){
        exit(ENOENT);
	}
	// Check if the link name exists
	if (check_file(tgt_dir_inode, target_filename) != NULL) {
		perror("LINKNAME EXISTS");
		exit(EEXIST);
	}

	// implement link
	if (!flag) {
		// HardLink
		link_inode = src;
		src->i_links_count++;
	} else {
		// softLink
		blocks_num = strlen(source_path) / EXT2_BLOCK_SIZE;
		if (strlen(source_path) % EXT2_BLOCK_SIZE != 0) {
			blocks_num ++;
		}

        // Initialize inode
        link_index = allocate_inode();
        if (link_index == 0) {
        	// No space for inode
            perror("NO SPACE FOR INODE");
            exit(ENOSPC);
        }

        link_inode = get_inode_by_num(link_index);
        link_inode->i_mode = EXT2_S_IFLNK;
		link_inode->i_uid = 0;
		link_inode->i_gid = 0;
		link_inode->osd1 = 0;
		link_inode->i_generation = 0;
		link_inode->i_file_acl = 0;
		link_inode->i_dir_acl = 0;
		link_inode->i_faddr = 0;
		link_inode->extra[0] = 0;
		link_inode->extra[1] = 0;
		link_inode->extra[2] = 0;
		link_inode->i_links_count = 1;
		link_inode->i_size = strlen(source_path);	
		// Initialize block.
		block_index = allocate_block(blocks_num);
		unsigned char *block = disk + block_index * EXT2_BLOCK_SIZE;
		// Copy path
    	memcpy(block, source_path, strlen(source_path));
    
    	// Initialize i_block
    	inode->i_block[0] = block_index;
		inode->i_blocks = blocks_num * 2;
	}
	// Set entry
	link_entry->inode = link_inode;
	link_entry->rec_len = entry_size(strlen(target_filename));
	link_entry->name_len = strlen(target_filename);
	link_entry->file_type = EXT2_FT_REG_FILE;
	strncpy(link_entry->name, target_filename, link_entry->name_len);
	add_entry_to_dir(tgt_dir_entry, link_entry);
	return 0;
}
