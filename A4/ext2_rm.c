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
    	fprintf(stderr,"Can not remove root\n");
		return ENOENT;
    }
    if (target_path[strlen(target_path) - 1] == '/'){
		perror("the target path is invalid");
		return ENOENT;
	}
    // check if the target path is abosolute path
    char *target_path = malloc(sizeof(char) * (strlen(argv[2]) + 1));
	strncpy(target_path, argv[2], strlen(argv[2]));
	target_path[strlen(argv[2])] = '\0';
	if(target_path[0] != '/'){
		printf("Error: the absolute path should start with '/'\n");
		return ENOENT;
	}

	// read the disk
	if(mount_ex2(argv[1]) == 1){
		perror("the disk is invalid");
		exit(ENOENT);
	};
	
	struct ext2_dir_entry *parent_dir;
	char *target_file_name;
	// if the input give the new file name
	target_file_name = strrchr(argv[3], '/');
	char *parent_dir_path = malloc(sizeof(char) * (strlen(target_path) - strlen(target_file_name) + 1));
	strncpy(parent_dir_path, target_path, strlen(target_path) - strlen(target_file_name));
	parent_dir_path[strlen(target_path) - strlen(target_file_name)] = '\0';
	target_file_name ++;
	parent_dir = find_parent_dir(parent_dir_path);
	free(parent_dir_path);
	inode_num = remove_entry(parent_dir, target_file_name);
	return 0;
} 
