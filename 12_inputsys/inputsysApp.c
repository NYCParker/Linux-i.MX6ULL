/* Linux
INPUT子系统
驱动和设备匹配
Reference:Alientek I.MX6U
 */

#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/input.h>

static struct input_event inputevent; //input_event 输入事件

int main(int argc, char *argv[])
{
    int fd, retval;
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

    while (1)
    {
        retval = read(fd, &inputevent, sizeof(inputevent));

        if (retval > 0)
        {
            if (inputevent.type = EV_KEY)
            {
                if (inputevent.code == 11)
                {
                    printf("key %d %s\r\n", inputevent.code, inputevent.value ? "press" : "release");
                }
            }
        }

        else
        {
            printf("failed\r\n");
        }
    }

    return 0;
}