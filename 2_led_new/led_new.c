/* Linux
LED GPIO driver(new)
no device tree
Reference:Alientek I.MX6U
 */

#include "led_new.h"

struct devstruct led_dev;

static int led_new_open(struct inode *inode, struct file *filp) //打开设备
{
    filp->private_data = &led_dev;
    printk("open led_new device in Linux!\r\n");
    return 0;
}

/* 应用程序调用read函数从设备中读取数据的时候此函数会执行
   buf：用户空间的内存
   cnt：要读取的字节数 
   return: 读取的字节数 为负读取失败*/
static ssize_t led_new_read(struct file *filp, char __user *buf, //从设备读取
                            size_t cnt, loff_t *offt)
{
    printk(".read function is void!\r\n");
    return 0;
}

/* 应用程序调用write函数从设备中读取数据的时候此函数会执行
   buf：应用程序要写入设备的数据
   cnt：要写入的字节数 
   return: 写入的字节数 为负写入失败*/
static ssize_t led_new_write(struct file *filp, //向设备写入
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
        if ((*databuf) == LED_ON)
        {
            regval &= ~(1 << 3);
            writel(regval, GPIO1_DR);
            printk("turn the led on!\r\n");
        }

        else if ((*databuf) == LED_OFF)
        {
            regval |= (1 << 3);
            writel(regval, GPIO1_DR);
            printk("turn the led off!\r\n");
        }
    }

    return 0;
}

static int led_new_release(struct inode *inode, struct file *filp)
{
    printk("Release led_new device in Linux! \r\n");
    return 0;
}

static struct file_operations led_new_fops = {
    .owner = THIS_MODULE,
    .open = led_new_open,
    .read = led_new_read,
    .write = led_new_write,
    .release = led_new_release,
};

static int __init led_new_init(void) //驱动注册入口函数
{
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

    //设备号分配
    alloc_chrdev_region(&led_dev.devid, 0, LED_COUNT, LED_NAME);
    led_dev.majorid = MAJOR(led_dev.devid);
    led_dev.minorid = MINOR(led_dev.devid);
    printk("major:%d minor:%d\r\n", led_dev.majorid, led_dev.minorid);

    //初始化cdev，向系统添加字符设备
    led_dev.cdev.owner = THIS_MODULE;
    cdev_init(&led_dev.cdev, &led_new_fops);
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
    printk("led_new driver register succeeded\r\n");
    return 0;
}

static void __exit led_new_exit(void) //驱动注册出口函数
{
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    cdev_del(&led_dev.cdev);                            //注销设备
    unregister_chrdev_region(led_dev.devid,LED_COUNT); //释放设备号

    device_destroy(led_dev.class, led_dev.devid);
    class_destroy(led_dev.class); //删除类和设备
    printk("led_new driver unregister succeeded\r\n");
}

module_init(led_new_init); //注册 模块加载函数
module_exit(led_new_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");


//cdev和device的区别
/* 在最低级别上,Linux系统中的每个设备都由struct device实例表示
设备结构包含设备模型核心对系统进行建模所需的信息
但是,大多数子系统会跟踪有关其托管设备的其他信息.结果,很少有设备用裸设备结构来表示.
取而代之的是,该结构(如kobject结构)通常嵌入在设备的更高级别表示中 */