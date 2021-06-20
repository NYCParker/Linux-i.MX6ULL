/* Linux
中断 按键GPIO触发 App
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

int main(int argc, char *argv[])
{
    int i;
    int fd, retval;
    char *filename;
    unsigned int arg;
    unsigned char key_info;

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
                        printf("key%d was pushed",i);
                    }
                }
            }
        }
    }
    close(fd);

    return retval;
}