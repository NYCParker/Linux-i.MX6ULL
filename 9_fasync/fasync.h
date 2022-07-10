
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
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/fcntl.h>

#define IRQKEY_COUNT 1       //设备号个数
#define IRQKEY_NAME "fasync" //设备名
#define KEYVALUE 0X01        //key按键默认值(按下0,默认1)
#define INVAKEY 0XFF         //无效按键值
#define KEY_NUM 1            //按键数量

/* IO（含中断）描述结构体 */
struct io_irqstruct
{
    int gpio; //gpio编号
    //char* name; //gpio名字（gpio申请的时候取的）
    char name[10];
    int irq_num;                         //中断编号
    char irqname[10];                    //中断名字
    irqreturn_t (*handler)(int, void *); //中断服务函数
    atomic_t value;                      //IO高低电平
    atomic_t ok_flag;                    //完成标志
};

/* key设备结构体 */
struct devstruct
{
    dev_t devid; //设备号
    int majorid;
    int minorid;
    struct cdev cdev; //Linux使用cdev结构体表示一个字符设备（内核用来在内部表示字符设备的结构）
    struct class *class;
    struct device *device;

    struct device_node *nd; //设备节点
    struct io_irqstruct io_irq[KEY_NUM];

    struct timer_list timer;  //定时器
    unsigned char curkey_num; //当前处理的按键号

    atomic_t key_info;
    struct fasync_struct *async_queue;  //异步通知相关结构体
};

extern struct devstruct irqkey_dev;

#endif