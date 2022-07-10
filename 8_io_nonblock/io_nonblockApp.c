/* Linux
中断 按键GPIO触发 App 读阻塞
Reference:Alientek I.MX6U
 */

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"

int main(int argc, char *argv[])
{
    int i;
    int count = 0;
    int fd, retval;
    char *filename;
    unsigned int arg;
    unsigned char key_info;
    fd_set readfds;
    struct pollfd fds;

    if (argc != 2)
    {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    /* 打开驱动文件 */
    fd = open(filename, O_RDWR);

    if (fd < 0)
    {
        printf("Can't open file %s\r\n", filename);
        return -1;
    }

    fds.fd = fd;
    fds.events = POLLIN;

    while (1)
    {
        retval = poll(&fds, 1, 500);

        if (retval)
        {
            retval = read(fd, &key_info, sizeof(key_info));
            if (retval < 0)
            {
                printf("wrong babe");
            }
            else
            {
                if (key_info)
                {
                    for (i = 0; i < 8; i++)
                    {
                        if ((key_info & (1 << i)) != 0)
                        {
                            printf("key%d was pushed\r\n", i);
                        }
                    }
                }
            }
        }
        else if (retval == 0) //超时
        {
            count++;
            if (count == 20)
            {
                printf("a few moments later...\n");
                count = 0;
            }
        }
    }

    close(fd);

    return retval;
}