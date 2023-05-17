#define FILENAME "/dev/aesdchar"
#define BUF_SIZE 1024

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "aesd_ioctl.h"

int main(int argc, char* argv[]){
	uint32_t write_cmd, write_cmd_offset;
	if (argc<2)
		write_cmd = 0;
	else
		write_cmd = atoi(argv[1]);

	if (argc<3)
		write_cmd_offset = 0;
	else
		write_cmd_offset = atoi(argv[2]);

	struct aesd_seekto seekto = {write_cmd, write_cmd_offset};

	printf("set to command %d offset %d\n", seekto.write_cmd, seekto.write_cmd_offset);

	char buf[BUF_SIZE];
	memset(&buf, 0, BUF_SIZE);

	int fd = open(FILENAME, O_CREAT | O_RDWR | O_APPEND | O_TRUNC | O_SYNC, 0644);

	if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) == 0)
		while (read(fd, &buf, BUF_SIZE)){
			printf("%s", buf);
			memset(&buf, 0, BUF_SIZE);
		}
	else
		printf("ioctl error\n");

	close(fd);
}