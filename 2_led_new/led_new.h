#ifndef _LED_NEW_H
#define _LED_NEW_H

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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LED_COUNT 1        //设备号个数
#define LED_NAME "led_new" //设备名
#define LED_OFF 0          //关灯命令
#define LED_ON 1           //关灯命令

/* i.mx6u 寄存器物理地址 */
#define CCM_CCGR1_BASE (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0X020E02F4)
#define GPIO1_DR_BASE (0X0209C000)
#define GPIO1_GDIR_BASE (0X0209C004)

/* 映射后的寄存器虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

/* 设备结构体 */
struct devstruct
{
    dev_t devid;
    int majorid;
    int minorid;
    struct cdev cdev; //Linux使用cdev结构体表示一个字符设备（内核用来在内部表示字符设备的结构）
    struct class *class;
    struct device *device;
};

extern struct devstruct led_dev;

#endif