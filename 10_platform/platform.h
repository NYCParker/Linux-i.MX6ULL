
#ifndef _IRQ_KEY_H
#define _IRQ_KEY_H

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
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LED_COUNT 1         //设备号个数
#define LED_NAME "alphaled" //设备名
#define LED_OFF 0           //关灯命令
#define LED_ON 1            //关灯命令

/* 设备结构体 */
struct devstruct
{
    dev_t devid; //设备号
    int majorid;
    int minorid;
    struct cdev cdev; //Linux使用cdev结构体表示一个字符设备（内核用来在内部表示字符设备的结构）
    struct class *class;
    struct device *device;

    struct device_node *nd; //设备节点
    int gpio;               //GPIO标号
};

extern struct devstruct led_dev;

#endif