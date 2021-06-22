/* Linux
platform 驱动和设备的分离
驱动和设备匹配
Reference:Alientek I.MX6U
 */

#include "platform.h"

struct devstruct led_dev;

static int led_open(struct inode *inode, struct file *filp) //打开设备
{
    filp->private_data = &led_dev;
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

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

static int my_dev_init(void)
{
    //首先找到设备树节点
    led_dev.nd = of_find_node_by_path("/gpioled");
    if (led_dev.nd == NULL)
    {
        printk("led node not find!\r\n");
        return -EINVAL;
    }

    led_dev.gpio = of_get_named_gpio(led_dev.nd, "led-gpio", 0); //获得gpio编号
    if (led_dev.gpio < 0)
    {
        printk("can't get gpio_number\r\n");
        return -EINVAL;
    }

    printk("gpio number:%d,\r\n", led_dev.gpio);

    gpio_request(led_dev.gpio, "led");      //向Linux申请GPIO并命名
    gpio_direction_output(led_dev.gpio, 1); //设置为输出，默认高电平（灭）

    return 0;
}
//设备树match
static const struct of_device_id led_of_match[] = { //Struct used for matching a device
    {.compatible = "alpha-led"},
    {}};

//如果驱动和设备match了，就执行probe（即设备树的compatible match了编写的驱动的led_of_match的compatible
static int led_probe(struct platform_device *dev)
{
    printk("led driver and device matched babe\r\n");

    //设备号分配
    alloc_chrdev_region(&led_dev.devid, 0, LED_COUNT, LED_NAME);
    led_dev.majorid = MAJOR(led_dev.devid);
    led_dev.minorid = MINOR(led_dev.devid);
    printk("major:%d minor:%d\r\n", led_dev.majorid, led_dev.minorid);

    //初始化设置， 这里只要初始化IO
    my_dev_init();

    //初始化cdev，向系统添加字符设备
    led_dev.cdev.owner = THIS_MODULE;
    cdev_init(&led_dev.cdev, &led_fops);
    cdev_add(&led_dev.cdev, led_dev.devid, LED_COUNT); //用于向Linux系统注册设备

    //创建类以及利用mdev自动创建设备节点
    led_dev.class = class_create(THIS_MODULE, LED_NAME);
    if (IS_ERR(led_dev.class))
    {
        return PTR_ERR(led_dev.class);
        printk("class error\r\n");
    }
    led_dev.device = device_create(led_dev.class, NULL, led_dev.devid, NULL, LED_NAME);
    if (IS_ERR(led_dev.device))
    {
        return PTR_ERR(led_dev.device);
        printk("device error\r\n");
    }
    printk("led driver register succeeded\r\n");

    return 0;
}

static int led_remove(struct platform_device *dev)
{
    gpio_set_value(led_dev.gpio, 1);
    cdev_del(&led_dev.cdev);                            //注销设备
    unregister_chrdev_region(led_dev.devid, LED_COUNT); //释放设备号
    device_destroy(led_dev.class, led_dev.devid);
    class_destroy(led_dev.class); //删除类和设备

    printk("led driver unregister succeeded\r\n");
    return 0;
}

//platform 驱动结构体
static struct platform_driver led_driver = {
    .driver = {
        .name = "led",                  //驱动名字（不用platform_device就不通过这个匹配了）
        .of_match_table = led_of_match, //设备树匹配表（是通过compatible匹配了）
    },
    .probe = led_probe,
    .remove = led_remove,
};

MODULE_DEVICE_TABLE(of,led_of_match);

static int __init led_init(void) //驱动注册入口函数
{
    return platform_driver_register(&led_driver);
}

static void __exit led_exit(void) //驱动注册出口函数
{
    platform_driver_unregister(&led_driver);
}

module_init(led_init); //注册 模块加载函数
module_exit(led_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");
