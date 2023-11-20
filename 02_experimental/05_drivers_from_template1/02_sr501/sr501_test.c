
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>

static int fd;

/*
 * ./button_test /dev/100ask_button0
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

	 /* 2. 打开文件 */
    int fd2 = open("/dev/100ask_led", O_RDWR);
    if (fd == -1)
    {
        printf("can not open file %s\n", argv[1]);
        return -1;
    }

	char buf[2]= {0};

	while (1)
	{
		if (read(fd, &val, 4) == 4)
		{
			if(val > 0x0)
			{
				buf[1] = 0;
			}else {
				buf[1] = 1;
			}
			printf("get button: 0x%x\n", val);
			ret = write(fd2, buf, 2);
		}
		else
			printf("while get button: -1\n");
	}
	
	close(fd);
	
	return 0;
}


