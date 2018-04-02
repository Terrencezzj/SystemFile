

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
	struct ext2_inode * id = (struct ext2_inode *)(disk + gd->bg_inode_table * EXT2_BLOCK_SIZE);
	unsigned char *block_bitmap = disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE;
	unsigned char *inode_bitmap = disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE;
	int i, j;
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", gd->bg_block_bitmap);
    printf("    inode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("    inode table: %d\n", gd->bg_inode_table);
    printf("    free blocks: %d\n", gd->bg_free_blocks_count);
    printf("    free inodes: %d\n", gd->bg_free_inodes_count);
    printf("    used_dirs: %d\n", gd->bg_used_dirs_count);
	
	
	// print block bitmap
	printf("Block bitmap: ");
	for(i = 0; i < sb->s_blocks_count / 8; i++){
		for(j = 0; j < 8; j++){
			printf("%d", (block_bitmap[i] & 1 << j) >> j);
		}
		printf(" ");
	}
	printf("\n");
	
	
	// Print Inode bitmap
	printf("Inode bitmap: ");
	for(i = 0; i < sb->s_inodes_count / 8; i++){
		for(j = 0; j < 8; j++){
			printf("%d", (inode_bitmap[i] & 1 << j) >> j);
		}
		printf(" ");
	}
	printf("\n\n");
	// print inode detail
	printf("Inodes:\n");
	int index = 0;
	char dtype = 'a';
	for(i = 0; i < sb->s_inodes_count / 8; i++){
		for(j = 0; j < 8; j++){
			if (inode_bitmap[i] & 1 << j){
				if((id[index].i_mode & EXT2_S_IFDIR) != 0){
					dtype = 'd';
				}
				else if((id[index].i_mode & EXT2_S_IFREG) != 0){
					dtype = 'f';
				}
				if ((index == EXT2_ROOT_INO-1 || index > EXT2_GOOD_OLD_FIRST_INO-1) && dtype != 'a'){
					printf("[%d] type: %c size: %d links: %d blocks: %d\n", index + 1, dtype, \
                        id[index].i_size, id[index].i_links_count, id[index].i_blocks);
					printf("[%d] Blocks:  %d\n", index + 1, id[index].i_block[0]);
				}
			}
			index ++;
		}
	}
	// print directory
	printf("\n\n");
	printf("Directory Blocks:\n");
	index = 0;
	for(i = 0; i < sb->s_inodes_count / 8; i++){
		for(j = 0; j < 8; j++){
			if (inode_bitmap[i] & 1 << j){
				if((id[index].i_mode & EXT2_S_IFDIR) != 0){
					int total_size = 0;
					int rec_len = 0;
					if (index == EXT2_ROOT_INO-1 || index > EXT2_GOOD_OLD_FIRST_INO-1){
						printf("    DIR BLOCK NUM: %d (for inode %d)\n", id[index].i_block[0], index + 1);
						struct ext2_dir_entry * cur_dir = (struct ext2_dir_entry *) (disk \
						 + id[index].i_block[0] * EXT2_BLOCK_SIZE);
						while(total_size < EXT2_BLOCK_SIZE){
							cur_dir = (void *)cur_dir + rec_len;
							char *file_name = malloc(cur_dir->name_len + 1);
							file_name = strncpy(file_name, cur_dir->name, cur_dir->name_len);
							file_name[cur_dir->name_len + 1] = '\0';
							if(cur_dir->file_type == EXT2_FT_DIR){
								printf("Inode: %d rec_len: %d name_len: %d type= d name=%s\n", \
								cur_dir->inode, cur_dir->rec_len, cur_dir->name_len, file_name);
							}
							if(cur_dir->file_type == EXT2_FT_REG_FILE){
								printf("Inode: %d rec_len: %d name_len: %d type= f name=%s\n", \
								cur_dir->inode, cur_dir->rec_len, cur_dir->name_len, file_name);
							}
							free(file_name);
							rec_len = cur_dir->rec_len;
							total_size += rec_len;						 						
						}
						
					}
				}
			}
			index ++;
		}
	}

    return 0;
}



