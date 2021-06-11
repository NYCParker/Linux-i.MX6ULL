
#ifndef _LED_GPIO_H
#define _LED_GPIO_H

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
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LED_COUNT 1         //设备号个数
#define LED_NAME "led_gpio" //设备名
#define LED_OFF 0           //关灯命令
#define LED_ON 1            //关灯命令

/* 设备结构体 */
struct devstruct
{
    dev_t devid;
    int majorid;
    int minorid;
    struct cdev cdev; //Linux使用cdev结构体表示一个字符设备（内核用来在内部表示字符设备的结构）
    struct class *class;
    struct device *device;
    struct device_node *nd; //设备节点
    int gpio_number;        //设备使用的GPIO编号
};

extern struct devstruct led_dev;

#endif