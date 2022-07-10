
/* Linux
LED GPIO driver
no device tree
Reference:Alientek I.MX6U
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define LED_MAJOR 1015     //主设备号
#define LED_NAME "led_old" //设备名

#define LEDOFF 0 //关灯
#define LEDON 1  //开灯

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

static int led_old_open(struct inode *inode, struct file *filp) //打开设备
{
    printk("open led_old device in Linux!\r\n");
    return 0;
}

/* 应用程序调用read函数从设备中读取数据的时候此函数会执行
   buf：用户空间的内存
   cnt：要读取的字节数 
   return: 读取的字节数 为负读取失败*/
static ssize_t led_old_read(struct file *filp, char __user *buf, //从设备读取
                            size_t cnt, loff_t *offt)
{
    printk(".read function is void!\r\n");
    return 0;
}

/* 应用程序调用write函数从设备中读取数据的时候此函数会执行
   buf：应用程序要写入设备的数据
   cnt：要写入的字节数 
   return: 写入的字节数 为负写入失败*/
static ssize_t led_old_write(struct file *filp, //向设备写入
                             const char __user *buf,
                             size_t cnt, loff_t *offt)
{
    int retvalue = 0;
    unsigned char databuf[1];
    uint32_t regval = 0;

    retvalue = __copy_from_user(databuf, buf, cnt);

    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    else
    {

        if ((*databuf) == LEDON)
        {
            regval &= ~(1 << 3);
            writel(regval, GPIO1_DR);
            printk("turn the led on!\r\n");
        }

        else if ((*databuf) == LEDOFF)
        {
            regval |= (1 << 3);
            writel(regval, GPIO1_DR);
            printk("turn the led off!\r\n");
        }
    }

    return 0;
}

static int led_old_release(struct inode *inode, struct file *filp)
{
    printk("Release led_old device in Linux! \r\n");
    return 0;
}

static struct file_operations led_old_fops = {
    .owner = THIS_MODULE,
    .open = led_old_open,
    .read = led_old_read,
    .write = led_old_write,
    .release = led_old_release,
};

static int __init led_old_init(void) //驱动注册入口函数
{
    int retvalue = 0;
    uint32_t regval = 0;

    //物理地址到虚拟地址映射
    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
    SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
    GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

    //i.mx6ull一系列GPIO输出初始化配置
    regval = readl(IMX6U_CCM_CCGR1);
    regval &= ~(3 << 26);
    regval |= (3 << 26);
    writel(regval, IMX6U_CCM_CCGR1); //使能时钟

    writel(5, SW_MUX_GPIO1_IO03);      //复用
    writel(0x10B0, SW_PAD_GPIO1_IO03); //IO属性

    regval = readl(GPIO1_GDIR);
    regval &= ~(1 << 3);
    regval |= (1 << 3);
    writel(regval, GPIO1_GDIR);

    regval = readl(GPIO1_DR);
    regval |= (1 << 3);
    writel(regval, GPIO1_DR);

    retvalue = register_chrdev(LED_MAJOR, LED_NAME, &led_old_fops); //注册设备

    if (retvalue < 0)
    {
        printk("led_old driver register failed\r\n");
    }
    else
    {
        printk("led_old driver register succeeded\r\n");
    }

    return 0;
}

static void __exit led_old_exit(void) //驱动注册出口函数
{
    unregister_chrdev(LED_MAJOR, LED_NAME); //注销设备
    printk("led_old driver unegister succeeded\r\n");
}

module_init(led_old_init); //注册 模块加载函数
module_exit(led_old_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");