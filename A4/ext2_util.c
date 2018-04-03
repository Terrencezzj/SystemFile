#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "ext2.h"
#include "ext2_util.h"

unsigned char *disk;
struct ext2_super_block *sb;
struct ext2_group_desc *gd;
void *inode_table;
unsigned char *block_bitmap;
unsigned char *inode_bitmap;

struct ext2_dir_entry *get_entry(struct ext2_inode *dir_inode, char *name){
	struct ext2_dir_entry * cur_dir;
	if (name == NULL){
		return NULL;
	}
	int name_length = strlen(name);
	if (name_length > EXT2_NAME_LEN){
		exit(1);
	}
	if ((dir_inode->i_mode & EXT2_S_IFDIR) == 0){
		printf("Error: some part of the path is not a directory\n");
		exit(1);
	}
	int i;
	for (i = 0; i < dir_inode->i_blocks / 2; i++){
		int total_size = 0;
		int rec_len = 0;
		if (i < 12){
		cur_dir = (struct ext2_dir_entry *) (disk + dir_inode->i_block[i] * EXT2_BLOCK_SIZE);
		}
		// the i_block is contained in the indirect pointer
		else{
			// break;
			unsigned int *indirect_block = (void *)(disk + (dir_inode->i_block[12]) * EXT2_BLOCK_SIZE);
			unsigned int block_index = indirect_block[i - 12] - 1;
			cur_dir = (struct ext2_dir_entry *) (disk + block_index * EXT2_BLOCK_SIZE);
		}
		while(total_size < EXT2_BLOCK_SIZE){
			cur_dir = (void *)cur_dir + rec_len;
			char *file_name = malloc(cur_dir->name_len + 1);
			strncpy(file_name, cur_dir->name, cur_dir->name_len);
			file_name[cur_dir->name_len] = '\0';
			// find the correspond directroy
			if(strcmp(file_name, name) == 0){
				if(cur_dir->file_type == EXT2_FT_DIR){
					return cur_dir;
				}
			}
			free(file_name);
			rec_len = cur_dir->rec_len;
			total_size += rec_len;						 						
		}

	}
	return NULL;
}

struct ext2_dir_entry *check_file(struct ext2_inode *dir_inode, char *name){
	struct ext2_dir_entry * cur_dir;
	if (name == NULL){
		return NULL;
	}
	int name_length = strlen(name);
	if (name_length > EXT2_NAME_LEN){
		exit(1);
	}
	if ((dir_inode->i_mode & EXT2_S_IFDIR) == 0){
		printf("Error: some part of the path is not a directory\n");
		exit(1);
	}
	int i;
	for (i = 0; i < dir_inode->i_blocks / 2; i++){
		int total_size = 0;
		int rec_len = 0;
		if (i < 12){
			cur_dir = (struct ext2_dir_entry *) (disk + dir_inode->i_block[i] * EXT2_BLOCK_SIZE);
		}
		// the i_block is contained in the indirect pointer
		else{
			// break;
			unsigned int *indirect_block = (void *)(disk + (dir_inode->i_block[12]) * EXT2_BLOCK_SIZE);
			unsigned int block_index = indirect_block[i - 12] - 1;
			cur_dir = (struct ext2_dir_entry *) (disk + block_index * EXT2_BLOCK_SIZE);
		}
		while(total_size < EXT2_BLOCK_SIZE){
			cur_dir = (void *)cur_dir + rec_len;
			char *file_name = malloc(cur_dir->name_len + 1);
			strncpy(file_name, cur_dir->name, cur_dir->name_len);
			file_name[cur_dir->name_len] = '\0';
			// find the correspond directroy
			if(strcmp(file_name, name) == 0){
				if(cur_dir->file_type == EXT2_FT_REG_FILE){
					return cur_dir;
				}
			}
			free(file_name);
			rec_len = cur_dir->rec_len;
			total_size += rec_len;						 						
		}

	}
	return NULL;
}

unsigned int remove_entry(struct ext2_inode *parent_inode, char *name){
	struct ext2_dir_entry * cur_dir;
	struct ext2_dir_entry * prev_dir;
	char *file_name;
	unsigned int result;
	if (name == NULL){
		return 0;
	}
	int name_length = strlen(name);
	if (name_length > EXT2_NAME_LEN){
		exit(1);
	}
	int i;
	for (i = 0; i < parent_inode->i_blocks / 2; i++){
		int total_size = 0;
		if (i < 12){
			prev_dir = (struct ext2_dir_entry *) (disk + parent_inode->i_block[i] * EXT2_BLOCK_SIZE);
		}
		// the i_block is contained in the indirect pointer
		else{
			// break;
			unsigned int *indirect_block = (void *)(disk + (parent_inode->i_block[12]) * EXT2_BLOCK_SIZE);
			unsigned int block_index = indirect_block[i - 12] - 1;
			prev_dir = (struct ext2_dir_entry *) (disk + block_index * EXT2_BLOCK_SIZE);
		}
		// if the delete enry is the first entry
		file_name = malloc(prev_dir->name_len + 1);
		strncpy(file_name, prev_dir->name, prev_dir->name_len);
		file_name[prev_dir->name_len] = '\0';
		// find the correspond directroy
		if((strcmp(file_name, name) == 0) && prev_dir->inode != 0){
			if((prev_dir->file_type == EXT2_FT_REG_FILE) ||\
			 (prev_dir->file_type == EXT2_FT_SYMLINK)){
				result = prev_dir->inode;
				prev_dir->inode = 0;
				// free(file_name);
				return result;
			}
		}
		free(file_name);
		while(total_size < EXT2_BLOCK_SIZE){
			cur_dir = (void *)prev_dir + prev_dir->rec_len;
			file_name = malloc(cur_dir->name_len + 1);
			strncpy(file_name, cur_dir->name, cur_dir->name_len);
			file_name[cur_dir->name_len] = '\0';
			// find the correspond directroy
			if((strcmp(file_name, name) == 0) && cur_dir->inode != 0){
				if((cur_dir->file_type == EXT2_FT_REG_FILE) || \
					(cur_dir->file_type == EXT2_FT_SYMLINK)){
					prev_dir->rec_len += cur_dir->rec_len;
					return cur_dir->inode;
				}
			}
			free(file_name);
			total_size += cur_dir->rec_len;
			prev_dir = (void *)prev_dir + prev_dir->rec_len;			 						
		}

	}
	return 0;

}
struct ext2_inode *get_inode_by_num(unsigned int num){
	if (num <= 0){
		return NULL;
	}
	else{
		struct ext2_inode *result = (struct ext2_inode *)(inode_table + (num - 1) * sizeof(struct ext2_inode));
		return result;
	}
}


// create a new directory and give it inode and give it block
int initialize_new_dir(struct ext2_dir_entry *parent, char *name){
	int name_len = strlen(name);
	// get the rec len
	unsigned short rec_len = entry_size(name_len);
	// assign new inode for the new dir
	unsigned int new_inode_index = allocate_inode();
	if (new_inode_index == 0){
		perror("Too many files in the disk");
		exit(1);
	}
	set_inode_bitmap(new_inode_index, 1);
	gd->bg_free_inodes_count -= 1;
	sb->s_free_inodes_count -= 1;
	// assign new block for the dir
	unsigned int *new_block_indexs = allocate_block(1);
	if (new_block_indexs == NULL){
		perror("No enough place for this file");
		exit(1);
	}
	set_block_bitmap(new_block_indexs[0], 1);
	gd->bg_free_blocks_count -= 1;
	sb->s_free_blocks_count -= 1;
	gd->bg_used_dirs_count ++;
	// link the inode of the new dir to the blocks
	set_inode(new_inode_index, EXT2_S_IFDIR, EXT2_BLOCK_SIZE, 1, 1, new_block_indexs);
	// add new dir entry in parent dir
	struct ext2_dir_entry *new_dir = malloc(sizeof(struct ext2_dir_entry));
	new_dir->inode = new_inode_index + 1;
	new_dir->name_len = name_len;
	new_dir->rec_len = rec_len;
	new_dir->file_type = EXT2_FT_DIR;
	strncpy(new_dir->name, name, name_len);
	add_entry_to_dir(parent, new_dir);

	// add "." dir entry in new dir
	struct ext2_dir_entry *new_block = (struct ext2_dir_entry *)(disk + (new_block_indexs[0] + 1) * EXT2_BLOCK_SIZE);
	new_block->inode = new_inode_index + 1;
	new_block->name_len = 1;
	new_block->rec_len = EXT2_BLOCK_SIZE;
	new_block->file_type = EXT2_FT_DIR;
	strncpy(new_block->name, ".", 1);
	struct ext2_inode *new_inode = get_inode_by_num(new_inode_index + 1);
	new_inode->i_links_count ++;
	// add ".." dir entry in new dir
	struct ext2_dir_entry *parent_dir = malloc(sizeof(struct ext2_dir_entry));
	parent_dir->inode = parent->inode;
	parent_dir->name_len = 2;
	parent_dir->rec_len = entry_size(2);
	parent_dir->file_type = EXT2_FT_DIR;
	strncpy(parent_dir->name, "..", 2);
	add_entry_to_dir(new_dir, parent_dir);
	struct ext2_inode *parent_inode = get_inode_by_num(parent->inode);
	parent_inode->i_links_count ++;

	//free
	free(new_dir);
	free(parent_dir);
	return 0;
}


unsigned short entry_size(unsigned char name_len){
	int rec_len = ENTRY_SIZE_EXCEPT_NAME + name_len;
	if (rec_len % 4 != 0){
		rec_len = ((rec_len / 4) + 1) * 4; 
	}
	return rec_len;
}


int add_entry_to_dir(struct ext2_dir_entry *parent_dir, struct ext2_dir_entry *new_dir){
	int i;
	struct ext2_inode *parent_inode = get_inode_by_num(parent_dir->inode);
	int parent_block_num = parent_inode->i_blocks / 2;
	struct ext2_dir_entry *new_dir_block;

	for (i = 0; i < parent_block_num; i++){
		int total_size = 0;
		int rec_len = 0;
		struct ext2_dir_entry *cur_dir;
		if(i < 12){
			cur_dir = (struct ext2_dir_entry *) (disk + (parent_inode->i_block[i]) * EXT2_BLOCK_SIZE);
		}
		else{
			unsigned int *indirect_block = (void *)(disk + (parent_inode->i_block[12]) * EXT2_BLOCK_SIZE);
			unsigned int block_index = indirect_block[i - 12] - 1;
			cur_dir = (struct ext2_dir_entry *) (disk \
			+ block_index * EXT2_BLOCK_SIZE);
		}

		while((total_size + cur_dir->rec_len) < EXT2_BLOCK_SIZE){
			cur_dir = (void *)cur_dir + cur_dir->rec_len;
			rec_len = cur_dir->rec_len;
			total_size += rec_len;
		}
		if (cur_dir->rec_len - entry_size(cur_dir->name_len) >= entry_size(new_dir->name_len)){
			int cur_dir_old_rec_len = cur_dir->rec_len;
			int cur_dir_new_rec_len = entry_size(cur_dir->name_len);
			int new_dir_rec_len = cur_dir_old_rec_len - cur_dir_new_rec_len;
			cur_dir->rec_len = cur_dir_new_rec_len;
			cur_dir = (void *)cur_dir + cur_dir_new_rec_len;
			// set the new entry into the end
			cur_dir->inode = new_dir->inode;
			cur_dir->rec_len = new_dir_rec_len;
			cur_dir->name_len = new_dir->name_len;
			cur_dir->file_type = new_dir->file_type;
			strncpy(cur_dir->name, new_dir->name, new_dir->name_len);
			printf("Inode: %d\n", cur_dir->inode);
			return 0;
		}
		else{
			break;
		}
	}
	// if all blocks are full
	unsigned int *new_free_block = allocate_block(1);
	if (new_free_block == NULL){
		printf("There is no block avaiable in the disk\n");
		exit(1);
	}
	set_block_bitmap(new_free_block[0], 1);
	gd->bg_free_blocks_count -= 1;
	sb->s_free_blocks_count -= 1;
	// put the inode into the parent inode
	if((parent_block_num) <= 11){
		parent_inode->i_block[parent_block_num] = new_free_block[0] + 1;
	}
	else{
		unsigned int *indirect_block = (void *)(disk + (parent_inode->i_block[12] - 1) * EXT2_BLOCK_SIZE);
		indirect_block[parent_block_num - 12] = new_free_block[0] + 1;
	}
	parent_inode->i_size += EXT2_BLOCK_SIZE;
	parent_inode->i_blocks += 2;
	new_dir_block = ((void*)disk + EXT2_BLOCK_SIZE * (new_free_block[0] + 1));
	new_dir_block->inode = new_dir->inode;
	new_dir_block->rec_len = EXT2_BLOCK_SIZE;
	new_dir_block->name_len = new_dir->name_len;
	new_dir_block->file_type = new_dir->file_type;
	strncpy(new_dir_block->name, new_dir->name, new_dir->name_len);
	free(new_free_block);
	return 0;
}


int set_inode(unsigned int new_inode_index, unsigned short filetype, unsigned int size, \
		unsigned short i_links_count, unsigned int i_blocks_num, \
		unsigned int *i_block){
	int i;
	struct ext2_inode *inode = (struct ext2_inode *)(inode_table + new_inode_index * sizeof(struct ext2_inode));
	inode->i_mode = filetype;
	inode->i_size = size;
	inode->i_links_count = i_links_count;
	inode->i_blocks = i_blocks_num * 2;
	inode->i_dtime = 0;
	inode->i_uid = 0;
	inode->i_gid = 0;
	inode -> i_generation = 0;
	if (i_blocks_num <= 12){
		for (i=0; i < i_blocks_num; i++){
			inode->i_block[i] = i_block[i] + 1;
		}
	}
	else{
		for (i=0; i < i_blocks_num - 1; i++){
			if (i < 12){
				inode->i_block[i] = i_block[i] + 1;
			}
			else{
				inode->i_block[12] = i_block[12] + 1;
				unsigned int *indirect_block = (void *)(disk + (inode->i_block[12]) * EXT2_BLOCK_SIZE);
				indirect_block[i - 12] = i_block[i + 1] + 1;
			}
		}
	}
	// for (i=0; i < i_blocks_num; i++){
	// 	if (i < 12){
	// 		inode->i_block[i] = i_block[i] + 1;
	// 	}
	// 	else{
	// 		//break;
	// 		unsigned int *indirect_block = (void *)(disk + (inode->i_block[12]) * EXT2_BLOCK_SIZE);
	// 		indirect_block[i - 12] = i_blocks[i] + 1;
	// 		//inode->i_block[12][i - 12] = i_block[i] + 1;
	// 	}
	// }
	return 0;
}


/*
* get the bit of the inode bitmap by index, return should be 1 or 0
*/
int get_inode_bitmap(unsigned int index){
	int offset = index % BYTE_LENGTH;
	int prefix = index / BYTE_LENGTH;
	if (inode_bitmap[prefix] & (1 << offset)){
		return 1;
	} else{
		return 0;
	}
}


/*
* set the index of the bitmap to num, num should be 1 or 0
*/
void set_inode_bitmap(unsigned int index, int num){
	int offset = index % 8;
	int prefix = index / 8;
	unsigned char *byte = inode_bitmap + prefix;
	if (num == 1){
		*byte |= 1UL << offset;
	}
	else{
		*byte &= ~(1UL << offset);
	}
}


/*
* get the bit of the inode bitmap by index, return should be 1 or 0
*/
int get_block_bitmap(unsigned int index){
	int offset = index % BYTE_LENGTH;
	int prefix = index / BYTE_LENGTH;
	if (block_bitmap[prefix] & (1 << offset)){
		return 1;
	} else{
		return 0;
	}
}


void set_block_bitmap(unsigned int index, int num){
	int offset = index % 8;
	int prefix = index / 8;
	unsigned char *byte = block_bitmap + prefix;
	if (num == 1){
		*byte |= 1UL << offset;
	}
	else{
		*byte &= ~(1UL << offset);
	}
}


unsigned int *allocate_block(int block_num){
	if(gd->bg_free_blocks_count < block_num){
		return NULL;
	}
	unsigned int *blocks_allocate = malloc(sizeof(unsigned int) * block_num);
	int already_allocate_num = 0;
	int i, j;
	unsigned int index = 0;
	for(i = 0; i < sb->s_blocks_count / 8; i++){
		for(j = 0; j < 8; j++){
			if((block_bitmap[i] & 1 << j) == 0){
				blocks_allocate[already_allocate_num] = index;
				already_allocate_num ++;
				if (already_allocate_num == block_num){
					return blocks_allocate;
				}
			}
			index ++;
		}
	}
	return NULL;
}


unsigned int allocate_inode(){
	unsigned int index = 0;
	int i, j;
	for (i = 0; i < sb->s_blocks_count / 32; i++){
		for (j = 0; j < 8; j++){
			if (((inode_bitmap[i] & 1 << j) == 0) && (index >= 11)){
				return index;
			}
			index ++;
		}
	}
	return 0;
}


struct ext2_dir_entry *find_parent_dir(char *path){
	struct ext2_inode *root_inode = (struct ext2_inode *)(inode_table + (EXT2_ROOT_INO - 1) * sizeof(struct ext2_inode));
	char *token = strtok(path, "/");
	struct ext2_dir_entry *cur_dir = (struct ext2_dir_entry *)(disk + (root_inode->i_block[0]) * EXT2_BLOCK_SIZE); 
	// if the path is "/", we return the root 
	if (token == NULL){
		return cur_dir;
	}
	// search through root inode to get the next entry
	while (token != NULL){
		cur_dir = get_entry(inode_table + (cur_dir->inode - 1) * sizeof(struct ext2_inode), token);
		if (cur_dir == NULL){
			printf("Error: the absolute path doesnot exist\n");
			exit(ENOENT);
		}
		token = strtok(NULL, "/");
	}
	return cur_dir;
};


struct ext2_inode *find_parent_inode(char *path){
	struct ext2_inode *cur_inode = (struct ext2_inode *)(inode_table + (EXT2_ROOT_INO - 1) * sizeof(struct ext2_inode));
	char *token = strtok(path, "/");
	struct ext2_dir_entry *cur_dir;
	// if the path is "/", we return the root inode
	if (token == NULL){
		return cur_inode;
	}
	// search through root inode to get the next entry
	while (token != NULL){
		cur_dir = get_entry(cur_inode, token);
		if (cur_dir == NULL){
			printf("Error: the absolute path doesnot exist\n");
			exit(ENOENT);
		}
		token = strtok(NULL, "/");
		cur_inode = get_inode_by_num(cur_dir->inode);
	}
	return cur_inode;
}


int mount_ex2(char *disk_path){
	int fd = open(disk_path, O_RDWR);
	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(disk == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
	sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
	gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
	inode_table = (struct ext2_inode *)(disk + gd->bg_inode_table * EXT2_BLOCK_SIZE);
	block_bitmap = disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE;
	inode_bitmap = disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE;
	return 0;
}

/*
 * Return if the path is an absolute path
 */
int is_not_absolute(char *path) {
	return !(path[0] == '/');
}


/*
* return the inode number of the file by given path
*/
int get_index(char *path){ 

	int i = EXT2_ROOT_INO;
	int lengh = strlen(path) + 1;
	char path_cp[lengh];
	char *next_dir;
	char *cur_dir;
	struct ext2_inode *cur_inode;

	if (is_not_absolute(path)) {
		fprintf(stderr, "NOT ABSOLUTE PATH ERROR");
		exit(EINVAL);
	}

	strncpy(path_cp, path, lengh);
	path_cp[lengh - 1] = '\0';
	next_dir = strtok(path_cp, "/");


	while(next_dir != NULL){
		cur_dir = next_dir;
		//printf("NEXT: %s\n", next_dir);
		next_dir = strtok(NULL, "/");
		// Check if i in the reversed space
		if( i > EXT2_ROOT_INO && i < EXT2_GOOD_OLD_FIRST_INO){
			exit(ENOENT);
		}

		cur_inode = get_inode_by_num(i);
		// check cur_inode
		if(cur_inode->i_size == 0){
			perror("TARGET FOUND SIZE ZERO");
			exit(ENOENT);
		}
		
		struct ext2_dir_entry *cur_dir_entry = get_entry(cur_inode, cur_dir);
		// check cur_dir_entry
		if (!cur_dir_entry) {
			if (next_dir) {
				perror("TARGET NOT FOUND");
				exit(ENOENT);
			}
			cur_dir_entry = check_file(cur_inode, cur_dir);
			if (!cur_dir_entry) {
				perror("TARGET NOT FOUND");
				exit(ENOENT);
			}
		} else if (!next_dir) {
			i = cur_dir_entry->inode;
			return i;
		} else if (cur_dir_entry->file_type != EXT2_FT_DIR) {
			perror("INVALID PATH");
            		exit(ENOENT);
		}
		i = cur_dir_entry->inode;
	}
	return i;
}


/*
 * Return if the imode of inode matched file_type of dir_entry
 */
int compare_mode_type(struct ext2_dir_entry *dir_entry, struct ext2_inode *inode) {
	if (((inode->i_mode & EXT2_S_IFREG) == EXT2_S_IFREG)
		&& (dir_entry->file_type == EXT2_FT_REG_FILE)) {
		return 1;
	} else if (((inode->i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR)
		&& (dir_entry->file_type == EXT2_FT_DIR)) {
		return 1;
	} else if (((inode->i_mode & EXT2_S_IFLNK) == EXT2_S_IFLNK)
		&& (dir_entry->file_type == EXT2_FT_SYMLINK)) {
		return 1;
	}
	return 0;
}
