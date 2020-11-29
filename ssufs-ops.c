#include "ssufs-ops.h"

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];
void ssufs_readSuperBlock(struct superblock_t*);
void ssufs_writeSuperBlock(struct superblock_t*);

int ssufs_allocFileHandle() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if (file_handle_array[i].inode_number == -1) {
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename){
	/* 1 */
	int i_index;
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));

	if(open_namei(filename) != -1)
		return -1;

	if((i_index = ssufs_allocInode()) == -1)
		return -1;

	ssufs_readInode(i_index, tmp);
	tmp->status = INODE_IN_USE;
	strcpy(tmp->name, filename);
	ssufs_writeInode(i_index, tmp);
	free(tmp);

	return i_index;
}

void ssufs_delete(char *filename){
	/* 2 */
	int i_index;

	if((i_index = open_namei(filename)) == -1)
		return ;

	ssufs_freeInode(i_index);
	return ;
}

int ssufs_open(char *filename){
	/* 3 */
	int i_index, t_index;
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));

	if((i_index = open_namei(filename)) == -1)
		return -1;

	ssufs_readInode(i_index, tmp); 
	t_index = ssufs_allocFileHandle();
	file_handle_array[t_index].inode_number = i_index;
	free(tmp);

	return t_index;
}

void ssufs_close(int file_handle){
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes){
	/* 4 */
}

int ssufs_write(int file_handle, char *buf, int nbytes){
	/* 5 */
	int rest;
	char buffer[BLOCKSIZE*NUM_INODE_BLOCKS];
	char new_buffer[BLOCKSIZE*NUM_INODE_BLOCKS];
	char input[BLOCKSIZE];
	char *block, *start;
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));

	memset(buffer, 0, BLOCKSIZE*NUM_INODE_BLOCKS);
	memset(new_buffer, 0, BLOCKSIZE*NUM_INODE_BLOCKS);
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	if(nbytes + tmp->file_size > BLOCKSIZE * NUM_INODE_BLOCKS)
		return -1;

	for(int i = 0; i < NUM_INODE_BLOCKS; i++)
		if(tmp->direct_blocks[i] != -1) {
			ssufs_readDataBlock(tmp->direct_blocks[i], block);
			strcat(buffer, block);
		}

	strcpy(new_buffer, buffer);
	strncpy(new_buffer+file_handle_array[file_handle].offset, buf, nbytes); 
	rest = strlen(new_buffer);
	start = new_buffer;

	for(int i = 0; i < NUM_INODE_BLOCKS; i++) {
		// 저장완료시 종료
		if(rest <= 0)
			break;
		// 공간이 없으면 새로운 블럭 할당
		if(tmp->direct_blocks[i] == -1) {
			tmp->direct_blocks[i] = ssufs_allocDataBlock(); // 공간 더없으면 예외처리해라!!!
			char * temp = (char*)malloc(BLOCKSIZE*sizeof(char));
			ssufs_writeDataBlock(tmp->direct_blocks[i], temp);
		}
		strncpy(input, start, BLOCKSIZE);
		ssufs_writeDataBlock(tmp->direct_blocks[i], input);
		start += BLOCKSIZE;
		rest -= BLOCKSIZE;
	}

	tmp->file_size = strlen(new_buffer);
	file_handle_array[file_handle].offset += nbytes;
	ssufs_writeInode(file_handle_array[file_handle].inode_number, tmp);
	free(tmp);
	return 0;
}

int ssufs_lseek(int file_handle, int nseek){
	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	int fsize = tmp->file_size;

	offset += nseek;

	if ((fsize == -1) || (offset < 0) || (offset > fsize)) {
		free(tmp);
		return -1;
	}

	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}
