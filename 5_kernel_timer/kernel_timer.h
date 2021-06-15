
#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define TIMER_COUNT 1                  //设备号个数
#define TIMER_NAME "timer"             //设备名
#define CLOSE_CMD (_IO(0XEF, 0x1))     //关闭定时器
#define OPEN_CMD (_IO(0XEF, 0x2))      //打开定时器
#define SETPERIOD_CMD (_IO(0XEF, 0x3)) // 设置定时器周期命令

/* timer设备结构体 */
//定时器本身的驱动也是要在驱动层编写，但是回调函数为何也在驱动层？
struct devstruct
{
    dev_t devid;
    int majorid;
    int minorid;
    struct cdev cdev; //Linux使用cdev结构体表示一个字符设备（内核用来在内部表示字符设备的结构）
    struct class *class;
    struct device *device;
    struct device_node *nd;  //设备节点
    int timer_period;        //定时周期，单位为ms
    struct timer_list timer; //定时器结构体
    spinlock_t lock;          //自旋锁
};

extern struct devstruct timer_dev;

#endif