#include "ssufs-ops.h"

int main()
{
	char str[] = "!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!";
	char str2[256];
	ssufs_formatDisk();

	ssufs_create("f1.txt");
	int fd1 = ssufs_open("f1.txt");

	printf("Write Data: %d\n", ssufs_write(fd1, str, BLOCKSIZE));
	printf("Seek: %d\n", ssufs_lseek(fd1, -40));
	printf("Read Data: %d\n", ssufs_read(fd1, str2, 32));
	printf("DATA : %s\n", str2);
	ssufs_dump();
	ssufs_delete("f1.txt");
	ssufs_dump();
}

