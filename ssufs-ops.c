#include "ssufs-ops.h"

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int ssufs_allocFileHandle() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if (file_handle_array[i].inode_number == -1) {
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename){
	int i_index;
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));

	// 예외처리
	if(open_namei(filename) != -1)
		return -1;
	if((i_index = ssufs_allocInode()) == -1)
		return -1;

	// inode 정보 세팅
	ssufs_readInode(i_index, tmp);
	tmp->status = INODE_IN_USE;
	strcpy(tmp->name, filename);
	ssufs_writeInode(i_index, tmp);
	free(tmp);

	return i_index;
}

void ssufs_delete(char *filename){
	int i_index;
	char *block = (char*)malloc(BLOCKSIZE*sizeof(char));
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));

	// 예외처리
	if((i_index = open_namei(filename)) == -1)
		return ;

	// file_handle_array 정리
	for(int i = 0; i < MAX_OPEN_FILES; i++)
		if(i_index == file_handle_array[i].inode_number) {
			file_handle_array[i].inode_number = -1;
			file_handle_array[i].offset = 0;
		}

	// disk 정리
	ssufs_readInode(i_index, tmp); 
	memset(tmp->name, 0, MAX_NAME_STRLEN);
	for(int i = 0; i < NUM_INODE_BLOCKS; i++) 
		if(tmp->direct_blocks[i] != -1) {
			ssufs_readDataBlock(tmp->direct_blocks[i], block);
			memset(block, 0, BLOCKSIZE);
			ssufs_writeDataBlock(tmp->direct_blocks[i], block);
		}

	ssufs_writeInode(i_index, tmp); 
	ssufs_freeInode(i_index);

	return ;
}

int ssufs_open(char *filename){
	int i_index, t_index;

	// 예외처리
	if((i_index = open_namei(filename)) == -1)
		return -1;
	if((t_index = ssufs_allocFileHandle()) < 0)
		return -1;

	file_handle_array[t_index].inode_number = i_index;

	return t_index;
}

void ssufs_close(int file_handle){
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes){
	char buffer[BLOCKSIZE*NUM_INODE_BLOCKS];
	char *block = (char*)malloc(BLOCKSIZE*sizeof(char));
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	memset(buffer, 0, BLOCKSIZE*NUM_INODE_BLOCKS);

	if(file_handle_array[file_handle].inode_number == -1)
		return -1;

	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	if(tmp->file_size < file_handle_array[file_handle].offset+nbytes)
		return -1;

	for(int i = 0; i < NUM_INODE_BLOCKS; i++)
		if(tmp->direct_blocks[i] != -1) {
			ssufs_readDataBlock(tmp->direct_blocks[i], block);
			strcat(buffer, block);
		}

	strncpy(buf, buffer+file_handle_array[file_handle].offset, nbytes);

	file_handle_array[file_handle].offset += nbytes;
	free(tmp);
	return 0;
}

int ssufs_write(int file_handle, char *buf, int nbytes){
	/* 5 */
	int rest;
	char buffer[BLOCKSIZE*NUM_INODE_BLOCKS];
	char new_buffer[BLOCKSIZE*NUM_INODE_BLOCKS];
	char input[BLOCKSIZE];
	char *block = (char*)malloc(BLOCKSIZE*sizeof(char));
	char *start;
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));

	memset(buffer, 0, BLOCKSIZE*NUM_INODE_BLOCKS);
	memset(new_buffer, 0, BLOCKSIZE*NUM_INODE_BLOCKS);

	if(file_handle_array[file_handle].inode_number == -1)
		return -1;

	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);

	if(nbytes + tmp->file_size > BLOCKSIZE * NUM_INODE_BLOCKS)
		return -1;

	for(int i = 0; i < NUM_INODE_BLOCKS; i++)
		if(tmp->direct_blocks[i] != -1) {
			ssufs_readDataBlock(tmp->direct_blocks[i], block);
			strcat(buffer, block);
		}

	strcpy(new_buffer, buffer);
	new_buffer[BLOCKSIZE*NUM_INODE_BLOCKS] = '\0';
	strncpy(new_buffer+file_handle_array[file_handle].offset, buf, nbytes); 
	rest = strlen(new_buffer);
	start = new_buffer;

	for(int i = 0; i < NUM_INODE_BLOCKS; i++) {
		// 저장완료시 종료
		if(rest <= 0)
			break;
		if(tmp->direct_blocks[i] == -1) { // 공간 없으면 새로운 블록 할당
			if((tmp->direct_blocks[i] = ssufs_allocDataBlock()) < 0) { // 할당할 블록 부족 시 롤백
				rest = strlen(buffer);
				start = buffer;
				for(int i = 0; i < NUM_INODE_BLOCKS; i++) {
					if(rest <= 0) { // 추가 할당한 블록 반납
						if(tmp->direct_blocks[i] != -1) {
							ssufs_readDataBlock(tmp->direct_blocks[i], block);
							memset(block, 0, BLOCKSIZE);
							ssufs_writeDataBlock(tmp->direct_blocks[i], block);
							ssufs_freeDataBlock(tmp->direct_blocks[i]);
						}
					}
					else {
						strncpy(input, start, BLOCKSIZE);
						ssufs_writeDataBlock(tmp->direct_blocks[i], input);
						start += BLOCKSIZE;
						rest -= BLOCKSIZE;
					}
				}
				free(tmp);
				return -1;
			}
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
