 #include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
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
    	fprintf(stderr,"Can not remove root\n");
		return ENOENT;
    }
    char *target_path = malloc(sizeof(char) * (strlen(argv[2]) + 1));
	strncpy(target_path, argv[2], strlen(argv[2]));
	target_path[strlen(argv[2])] = '\0';
    if (target_path[strlen(target_path) - 1] == '/'){
		perror("the target path is invalid");
		return ENOENT;
	}
    // check if the target path is abosolute path
	if(target_path[0] != '/'){
		printf("Error: the absolute path should start with '/'\n");
		return ENOENT;
	}

	// read the disk
	if(mount_ex2(argv[1]) == 1){
		perror("the disk is invalid");
		exit(ENOENT);
	};
	
	struct ext2_inode *parent_inode;
	char *target_file_name;
	// if the input give the new file name
	target_file_name = strrchr(argv[2], '/');
	char *parent_dir_path = malloc(sizeof(char) * (strlen(target_path) - strlen(target_file_name) + 1));
	strncpy(parent_dir_path, target_path, strlen(target_path) - strlen(target_file_name));
	parent_dir_path[strlen(target_path) - strlen(target_file_name)] = '\0';
	target_file_name ++;
	parent_inode = find_parent_inode(parent_dir_path);
	free(parent_dir_path);
	unsigned int inode_num = remove_entry(parent_inode, target_file_name);
	if (inode_num == 0){
		perror(target_file_name);
		// free(parent_dir_path);
		// free(target_path);
		exit(ENOENT);
	}

	struct ext2_inode *remove_inode = get_inode_by_num(inode_num);
	remove_inode->i_links_count --;
	if(remove_inode->i_links_count > 0){
		return 0;
	}
	// free the disk space for the file
	remove_inode->i_dtime = time(NULL);
	int i;
	int block_num = remove_inode->i_blocks / 2;
	for(i = 0; i < block_num; i++){
		if(i < 12){
			set_block_bitmap(remove_inode->i_block[i] - 1, 0);
			gd->bg_free_blocks_count ++;
			sb->s_free_blocks_count ++;
		}
		else{
			unsigned int *indirect_block = (void *)(disk + EXT2_BLOCK_SIZE * remove_inode->i_block[12]);
			set_block_bitmap(indirect_block[i - 12] - 1, 0);
			gd->bg_free_blocks_count ++;
			sb->s_free_blocks_count ++;
		}
	}
	set_inode_bitmap(inode_num - 1, 0);
	gd->bg_free_inodes_count ++;
	sb->s_free_inodes_count ++;
	free(parent_dir_path);
	free(target_path);
	return 0;
}