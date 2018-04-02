#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "ext2.h"
#include "ext2_util.h"

int main(int argc, char **argv) {

	// check the argument num
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <disk file name><absolute path>\n", argv[0]);
        exit(1);
    }
    // check if we want to make root 
    if (strcmp(argv[2],"/")==0){
    	fprintf(stderr,"Can not make root\n");
		return ENOENT;
    }
    // check if the target path is abosolute path
    char *target_path = argv[2];
	if(target_path[0] != '/'){
		printf("Error: the absolute path should start with '/'\n");
		return ENOENT;
	}
	// read the disk
	mount_ex2(argv[1]);
	// get the root inode
	struct ext2_inode *root_inode = (struct ext2_inode *)(inode_table + (EXT2_ROOT_INO - 1) * sizeof(struct ext2_inode));
	char * token = strtok(target_path, "/");
	struct ext2_dir_entry *prev_dir = (struct ext2_dir_entry *)(disk + (root_inode->i_block[0]) * EXT2_BLOCK_SIZE); 
	// search through root inode to get the next entry
	struct ext2_dir_entry *cur_dir = get_entry(root_inode, token);

	while (token != NULL){
		char *next_token = strtok(NULL, "/");
		if (next_token == NULL){
			// we got to the end of the path, create new dir
			if (cur_dir == NULL){
				struct  ext2_dir_entry *parent_dir = prev_dir;
				int new_dir = initialize_new_dir(parent_dir, token);
				return 0;
			}
			//some component on the path to the location 
			//where the final directory is to be created does not exist
			else{
				printf("Error: the directory already exist\n");
				return EEXIST;
			}
		}
		if (cur_dir == NULL){
			printf("Error: the absolute path doesnot exist\n");
			return ENOENT;
		}
		token = next_token;
		prev_dir = cur_dir;
		cur_dir = get_entry(inode_table + (cur_dir->inode - 1) * sizeof(struct ext2_inode), token);
	}
	return ENOENT;
}