
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * ./hello_drv_test -w abc
 * ./hello_drv_test -r
 */
int main(int argc, char **argv)
{
	int fd;
	
	int len;
	
	/* 1. 判断参数 */
	if (argc < 2) 
	{
		printf("Usage: %s -w <string>\n", argv[0]);
		printf("       %s -r\n", argv[0]);
		return -1;
	}

	/* 2. 打开文件 */
	fd = open("/dev/100ask_led", O_RDWR);
	if (fd == -1)
	{
		printf("can not open file /dev/hello\n");
		return -1;
	}
	char buf;
	/* 3. 写文件或读文件 */
	if (0 == strcmp(argv[1], "on"))
	{
		buf = 0x01;
		
	}else  if(0 == strcmp(argv[1], "off")){
		buf = 0x00;
	}

	write(fd, &buf, 1);
	
	
	close(fd);
	
	return 0;
}


