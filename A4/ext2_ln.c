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
	bool flag = false;
	int src_index;
	int tgt_dir_index;
	int exist_link;
	struct ext2_inode *src;
	struct ext2_inode *tgt_dir;

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
	src = inode_table + sizeof(struct ext2_inode) * (src_index - 1);
	//tgt_dir = inode_table + sizeof(struct ext2_inode) * (tgt_dir_index - 1);
	if (!flag && (src->i_mode & EXT2_S_IFDIR)) {
		perror("HARDLINK TO DIR");
		exit(EISDIR);
	}

	target_dir_path = dirname(target_path);
	tgt_dir_index = get_index(target_dir_path);
	tgt_dir = inode_table + sizeof(struct ext2_inode) * (target_dir_path - 1);

	// implement link
	if (flag) {
		int block_num = strlen(source_path) / EXT2_BLOCK_SIZE;

	} else {
		
	}

	






	return 0;
}
