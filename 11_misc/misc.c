/* Linux
misc 杂项驱动
驱动和设备匹配
Reference:Alientek I.MX6U
 */

#include "misc.h"

struct devstruct miscled_dev;

static int led_open(struct inode *inode, struct file *filp) //打开设备
{
    filp->private_data = &miscled_dev;
    printk("open led device in Linux!\r\n");
    return 0;
}

/* 应用程序调用read函数从设备中读取数据的时候此函数会执行
   buf：用户空间的内存
   cnt：要读取的字节数 
   return: 读取的字节数 为负读取失败*/
static ssize_t led_read(struct file *filp, char __user *buf, //从设备读取
                        size_t cnt, loff_t *offt)
{
    printk(".read function is void!\r\n");
    return 0;
}

/* 应用程序调用write函数从设备中读取数据的时候此函数会执行
   buf：应用程序要写入设备的数据
   cnt：要写入的字节数 
   return: 写入的字节数 为负写入失败*/
static ssize_t led_write(struct file *filp, //向设备写入
                         const char __user *buf,
                         size_t cnt, loff_t *offt)
{
    int retvalue = 0;
    unsigned char databuf[1];
    struct devstruct *dev = filp->private_data;

    retvalue = __copy_from_user(databuf, buf, cnt);

    if (retvalue < 0)
    {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    else
    {
        if ((*databuf) == LED_ON)
        {
            gpio_set_value(dev->gpio, 0);
            printk("turn the led on!\r\n");
        }

        else if ((*databuf) == LED_OFF)
        {
            gpio_set_value(dev->gpio, 1);
            printk("turn the led off!\r\n");
        }
    }

    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    printk("Release led device in Linux! \r\n");
    return 0;
}

static struct file_operations miscled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

static int my_dev_init(void)
{
    //首先找到设备树节点
    miscled_dev.nd = of_find_node_by_path("/gpioled");
    if (miscled_dev.nd == NULL)
    {
        printk("led node not find!\r\n");
        return -EINVAL;
    }

    miscled_dev.gpio = of_get_named_gpio(miscled_dev.nd, "led-gpio", 0); //获得gpio编号
    if (miscled_dev.gpio < 0)
    {
        printk("can't get gpio_number\r\n");
        return -EINVAL;
    }

    printk("gpio number:%d,\r\n", miscled_dev.gpio);

    gpio_request(miscled_dev.gpio, "led");      //向Linux申请GPIO并命名
    gpio_direction_output(miscled_dev.gpio, 1); //设置为输出，默认高电平（灭）

    return 0;
}

static struct miscdevice led_miscdev = {
    .minor = MISCLED_MINOR,
    .name = MISCLED_NAME,
    .fops = &miscled_fops,
};

//设备树match
static const struct of_device_id miscled_of_match[] = { //Struct used for matching a device
    {.compatible = "alpha-led"},
    {}};

//如果驱动和设备match了，就执行probe（即设备树的compatible match了编写的驱动的led_of_match的compatible
static int miscled_probe(struct platform_device *dev)
{
    int retval;
    printk("miscled driver and device matched babe\r\n");
    my_dev_init();

    retval = misc_register(&led_miscdev);
    if (retval < 0)
    {
        printk("miscled driver register failed!\r\n");
        return -EFAULT;
    }

    printk("miscled driver register succeeded\r\n");
    return 0;
}

static int miscled_remove(struct platform_device *dev)
{
    gpio_set_value(miscled_dev.gpio, 1);
    misc_deregister(&led_miscdev);
    printk("miscled driver unregister succeeded\r\n");
    return 0;
}

//platform 驱动结构体
static struct platform_driver miscled_driver = {
    .driver = {
        .name = "miscled",                  //驱动名字（不用platform_device就不通过这个匹配了）
        .of_match_table = miscled_of_match, //设备树匹配表（是通过compatible匹配了）
    },
    .probe = miscled_probe,
    .remove = miscled_remove,
};

static int __init miscled_init(void) //驱动注册入口函数
{
    return platform_driver_register(&miscled_driver);
}

static void __exit miscled_exit(void) //驱动注册出口函数
{
    platform_driver_unregister(&miscled_driver);
}

module_init(miscled_init); //注册 模块加载函数
module_exit(miscled_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");
