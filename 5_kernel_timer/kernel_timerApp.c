/* Linux
Kernel Timer 内核定时器 App
Reference:Alientek I.MX6U
 */

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

#define CLOSE_CMD (_IO(0XEF, 0x1))     //关闭定时器
#define OPEN_CMD (_IO(0XEF, 0x2))      //打开定时器
#define SETPERIOD_CMD (_IO(0XEF, 0x3)) // 设置定时器周期命令

int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];

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

    while (1)
    {
        printf("Input CMD:");
        retvalue = scanf("%d", &cmd);

        if (retvalue != 1)
        {
            gets(str);
        }

        if (cmd == 1)
        {
            cmd = CLOSE_CMD;
        }

        else if (cmd == 2)
        {
            cmd = OPEN_CMD;
        }

        else if (cmd == 3)
        {
            cmd = SETPERIOD_CMD;
            printf("setperiod cmd\r\n");
            printf("Input Timer Period:");
            retvalue = scanf("%d", &arg);
            if (retvalue != 1)
            {
                gets(str);
            }
        }

        ioctl(fd, cmd, arg);
    }
    close(fd);
}