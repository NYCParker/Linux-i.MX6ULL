/* Linux
misc 杂项驱动
驱动和设备匹配
Reference:Alientek I.MX6U
 */

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char *argv[])
{
    int fd, retvalue;
    char *filename;
    char databuf[1];

    if (argc != 4)
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

    if (atoi(argv[2]) == 1)
    { /* 从驱动文件读取数据 */
        retvalue = read(fd, databuf, 1);
        if (retvalue < 0)
        {
            printf("read file %s failed!\r\n", filename);
        }
        else
        {
            /* 读取成功，打印出读取成功的数据 */
            printf("read data:%s\r\n", databuf);
        }
    }

    if (atoi(argv[2]) == 2)
    {
        databuf[0]=atoi(argv[3]);
        retvalue = write(fd, databuf, sizeof(databuf));
        if (retvalue < 0)
        {
            printf("write file %s failed!\r\n", filename);
        }
    }
    /* 关闭驱动文件 */
    retvalue = close(fd);
    if (retvalue < 0)
    {
        printf("Can't close file %s\r\n", filename);
        return -1;
    }
    return 0;
}