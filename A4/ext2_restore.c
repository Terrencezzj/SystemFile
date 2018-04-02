#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include "ext2.h"
#include "ext2_util.h"

#define USAGE "Usage: %s <image file name> <absolute restore path>\n"

int main(int argc, char **argv) {
	// Variables
    unsigned int parent_inode_num;
    unsigned int restore_parent_index;
    struct ext2_inode *parent_inode;
    struct ext2_dir_entry *parent_entry;
    unsigned int dir_block_num;
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
        return EISDIR;
	}

	disk_image_path = argv[1];
	mount_ex2(disk_image_path);

	restore_parent = dirname(restore_path);
	restore_filename = basename(restore_path);
	restore_parent_index = get_index(restore_parent);
	parent_inode = get_inode_by_num(restore_parent_index);
	parent_entry = find_parent_dir(restore_parent);

	// Check if parent is directory
	if(!parent_inode->i_mode & EXT2_S_IFDIR){
        exit(ENOENT);
	}

	// Check if the file to restore exists (Not removed)
	if (check_file(parent_inode, restore_filename) != NULL) {
		exit(EEXIST);
	}



}
