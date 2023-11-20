
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int fd;

/*
 * ./led_test <led number> [on | off]
 *
 */
int main(int argc, char **argv)
{
    int val;
    int ret;
    char buf[2] = {};

    /* 1. 判断参数 */
    if (argc < 2)
    {
        printf("Usage: %s <led number> [on | off]\n", argv[0]);
        return -1;
    }

    /* 2. 打开文件 */
    fd = open("/dev/100ask_led", O_RDWR);
    if (fd == -1)
    {
        printf("can not open file %s\n", argv[1]);
        return -1;
    }
    if (argc == 3)
    {
        buf[0] = strtol(argv[1], NULL, 0);
        if (!strcmp(argv[2], "on"))
        {
            buf[1] = 0;
        }
        else
        {
            buf[1] = 1;
        }
        ret = write(fd, buf, 2);
        printf("write\n");
    }
    else
    {
        buf[0] = strtol(argv[1], NULL, 0);
        ret = read(fd, buf, 2);
        perror("read");
        printf("ret = %d, led%d status is %s\n", ret, buf[0], buf[1] == 1 ? "on" : "off");
        
    }

    close(fd);

    return 0;
}
