/* Linux
中断 按键GPIO触发 App 异步通知
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
#include "signal.h"

static int fd = 0;

static void sigio_signal_func(int signum)
{
    int i;
    int retval = 0;
    unsigned int key_info = 0;

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

int main(int argc, char *argv[])
{
    int flags = 0;
    char *filename;

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

    signal(SIGIO, sigio_signal_func);
    fcntl(fd, F_SETOWN, getpid());       //将当前进程的进程号告诉给内核
    flags = fcntl(fd, F_GETFD);             //获取当前的进程状态
    fcntl(fd, F_SETFL, flags | FASYNC); // 设置进程启用异步通知功能

    while (1)
    {
        sleep(2);
    }

    close(fd);

    return 0;
}