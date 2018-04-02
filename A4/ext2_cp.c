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
    if(argc != 4) {
        fprintf(stderr, "Usage: %s <disk file name><source_path><target path>\n", argv[0]);
        exit(1);
    }

    // check if the target path is abosolute path
    char *target_path = malloc(sizeof(char) * (strlen(argv[3]) + 1));
	strncpy(target_path, argv[3], strlen(argv[3]));
	target_path[strlen(argv[3])] = '\0';
	if(target_path[0] != '/'){
		printf("Error: the absolute path should start with '/'\n");
		return ENOENT;
	}

	// read the disk
	if(mount_ex2(argv[1]) == 1){
		perror("the disk is invalid");
		exit(ENOENT);
	};
	//check if the source path is invalid
	FILE *source_path = fopen(argv[2], "r");
	if(source_path == NULL){
		perror("the source file is invalid");
		exit(ENOENT);
	}
	// get the source file size
	struct stat fileStat;
	if(stat(argv[2], &fileStat) < 0){
		return 1;
	}
	unsigned long filesize = fileStat.st_size;
	int file_blocks_num = filesize / EXT2_BLOCK_SIZE + 1;
	// allocate inodes for the source file
	int file_inode_index = allocate_inode();
	if (file_inode_index == 0){
		perror("Too many files in the disk");
		exit(1);
	}
	set_inode_bitmap(file_inode_index, 1);
	gd->bg_free_inodes_count -= 1;
	sb->s_free_inodes_count -= 1;

	// allocate blocks for the source file
	int copy_num = file_blocks_num;
	if(file_blocks_num > 12){
		file_blocks_num += 1;
	}
	unsigned int* file_blocks = allocate_block(file_blocks_num);
	if (file_blocks == NULL){
		perror("No enough place for this file");
		exit(1);
	}
	gd->bg_free_blocks_count -= file_blocks_num;
	sb->s_free_blocks_count -= file_blocks_num;
	// copy data
	unsigned char *source_file_p = mmap(NULL, filesize, PROT_READ | PROT_WRITE, \
		MAP_PRIVATE, fileno(source_path), 0);
	int i;
	for(i = 0; i < copy_num - 1; i++){
		set_block_bitmap(file_blocks[i], 1);
		memcpy(disk + (file_blocks[i] + 1) * EXT2_BLOCK_SIZE, source_file_p, EXT2_BLOCK_SIZE);
		source_file_p += EXT2_BLOCK_SIZE;
	}
	set_block_bitmap(file_blocks[copy_num - 1], 1);
	memcpy(disk + (file_blocks[copy_num - 1] + 1) * EXT2_BLOCK_SIZE, source_file_p,\
	 filesize - (copy_num - 1) * EXT2_BLOCK_SIZE);
	if(file_blocks_num > 12){
		set_block_bitmap(file_blocks[file_blocks_num - 1], 1);
		unsigned int *indirect_block = (void *)(disk + (file_blocks[file_blocks_num - 1])\
		 * EXT2_BLOCK_SIZE);
		for(i = 12; i < copy_num; i++){
			*indirect_block = file_blocks[i] + 1;
			indirect_block ++;
		}
	}
	// get the target path
	struct ext2_dir_entry *parent_dir;
	char *new_name;
	// if the input ends with '/' we just give the file same name as before
	if (target_path[strlen(target_path) - 1] == '/'){
		parent_dir = find_parent_dir(target_path);
		new_name = strrchr(argv[2], '/');
		new_name ++;
	}
	// if the input give the new file name
	else{
		new_name = strrchr(argv[3], '/');
		char *parent_dir_path = malloc(sizeof(char) * (strlen(target_path) - strlen(new_name) + 1));
		strncpy(parent_dir_path, target_path, strlen(target_path) - strlen(new_name));
		parent_dir_path[strlen(target_path) - strlen(new_name)] = '\0';
		parent_dir = find_parent_dir(parent_dir_path);
		free(parent_dir_path);
		new_name ++;
	}
	// link the inode of the new dir to the blocks
	if(check_file(get_inode_by_num(parent_dir->inode), new_name) != NULL){
		perror("File with same name have already existed");
		return EEXIST;
	}
	set_inode(file_inode_index, EXT2_S_IFREG, filesize, 1, file_blocks_num, file_blocks);
	struct ext2_dir_entry *file_entry = malloc(sizeof(struct ext2_dir_entry));
	file_entry->inode = file_inode_index + 1;
	file_entry->rec_len = entry_size(strlen(new_name));
	file_entry->name_len = strlen(new_name);
	file_entry->file_type = EXT2_FT_REG_FILE;
	strncpy(file_entry->name, new_name, strlen(new_name));
	printf("%s\n", file_entry->name);
	add_entry_to_dir(parent_dir, file_entry);
	// free
	free(file_blocks);
	free(target_path);
	fclose(source_path);
	return 0;
} 