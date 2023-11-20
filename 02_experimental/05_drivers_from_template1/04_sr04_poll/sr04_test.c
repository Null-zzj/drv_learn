
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>
static int fd;
#define CMD_TRIG 100

/*
 * ./button_test /dev/sr04
 *
 */
int main(int argc, char **argv)
{
	int val;
	struct pollfd fds[1];
	int timeout_ms = 5000;
	int ret;
	int	flags;

	int i;
	
	/* 1. 判断参数 */
	if (argc != 2) 
	{
		printf("Usage: %s <dev>\n", argv[0]);
		return -1;
	}


	/* 2. 打开文件 */
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
	{
		printf("can not open file %s\n", argv[1]);
		return -1;
	}

	while (1)
	{
		ioctl(fd, CMD_TRIG);
		if (read(fd, &val, 4) == 4)
		{
			printf("get distence: %dcm\n", val * 17 / 1000000);
			sleep(1);
		}
		else
			printf("get distence err: -1\n");
	}
	
	close(fd);
	
	return 0;
}


