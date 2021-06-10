/* Linux
LED GPIO driver
device tree
在根节点下增加设备树节点dtsled
Reference:Alientek I.MX6U
 */

#include "led_dts.h"

struct devstruct led_dev;

static int led_dts_open(struct inode *inode, struct file *filp) //打开设备
{
    filp->private_data = &led_dev;
    printk("open led_dts device in Linux!\r\n");
    return 0;
}

/* 应用程序调用read函数从设备中读取数据的时候此函数会执行
   buf：用户空间的内存
   cnt：要读取的字节数 
   return: 读取的字节数 为负读取失败*/
static ssize_t led_dts_read(struct file *filp, char __user *buf, //从设备读取
                            size_t cnt, loff_t *offt)
{
    printk(".read function is void!\r\n");
    return 0;
}

/* 应用程序调用write函数从设备中读取数据的时候此函数会执行
   buf：应用程序要写入设备的数据
   cnt：要写入的字节数 
   return: 写入的字节数 为负写入失败*/
static ssize_t led_dts_write(struct file *filp, //向设备写入
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

static int led_dts_release(struct inode *inode, struct file *filp)
{
    printk("Release led_dts device in Linux! \r\n");
    return 0;
}

static struct file_operations led_dts_fops = {
    .owner = THIS_MODULE,
    .open = led_dts_open,
    .read = led_dts_read,
    .write = led_dts_write,
    .release = led_dts_release,
};

static int __init led_dts_init(void) //驱动注册入口函数
{
    int retval = 0;
    const char *poutstr;        //指向字符串的首地址
    struct property *pproperty; //property结构体表示属性
    uint32_t regdata[10];
    uint32_t regval = 0;

    //设备树的设备节点 用起来
    led_dev.nd = of_find_node_by_path("/dtsled"); //找到根节点下的dtsled节点
    if (led_dev.nd == NULL)
    {
        printk("dtsled node not found babe\r\n");
        return -EINVAL;
    }
    else
    {
        printk("dtsled node found babe\r\n");
    }

    //找到节点后，找到compatible属性
    pproperty = of_find_property(led_dev.nd, "compatible", NULL); //获取compatible属性内容
    if (pproperty == NULL)
    {
        printk("compatible property not found babe\r\n");
    }
    else
    {
        printk("compatible property found babe, compatible = %s\r\n", (char *)(pproperty->value));
    }

    //获取status属性
    retval = of_property_read_string(led_dev.nd, "status", &poutstr);
    if (retval < 0)
    {
        printk("status property not read babe\r\n");
    }
    else
    {
        printk("status property read babe, status = %s\r\n", poutstr);
    }

    //获取reg属性
    retval = of_property_read_u32_array(led_dev.nd, "reg", regdata, 10);
    if (retval < 0)
    {
        printk("reg property not read babe\r\n");
    }
    else
    {
        uint8_t i;
        printk("reg data:\r\n");
        for (i = 0; i < 10; i++)
        {
            printk("%#X ", regdata[i]);
        }
        printk("\r\n");
    }

    //物理地址到虚拟地址映射
    IMX6U_CCM_CCGR1 = of_iomap(led_dev.nd, 0);
    SW_MUX_GPIO1_IO03 = of_iomap(led_dev.nd, 1);
    SW_PAD_GPIO1_IO03 = of_iomap(led_dev.nd, 2);
    GPIO1_DR = of_iomap(led_dev.nd, 3);
    GPIO1_GDIR = of_iomap(led_dev.nd, 4);

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
    cdev_init(&led_dev.cdev, &led_dts_fops);
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
    printk("led_dts driver register succeeded\r\n");
    return 0;
}

static void __exit led_dts_exit(void) //驱动注册出口函数
{
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(SW_MUX_GPIO1_IO03);
    iounmap(SW_PAD_GPIO1_IO03);
    iounmap(GPIO1_DR);
    iounmap(GPIO1_GDIR);

    cdev_del(&led_dev.cdev);                            //注销设备
    unregister_chrdev_region(led_dev.devid, LED_COUNT); //释放设备号

    device_destroy(led_dev.class, led_dev.devid);
    class_destroy(led_dev.class); //删除类和设备
    printk("led_dts driver unregister succeeded\r\n");
}

module_init(led_dts_init); //注册 模块加载函数
module_exit(led_dts_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");

//cdev和device的区别
/* 在最低级别上,Linux系统中的每个设备都由struct device实例表示
设备结构包含设备模型核心对系统进行建模所需的信息
但是,大多数子系统会跟踪有关其托管设备的其他信息.结果,很少有设备用裸设备结构来表示.
取而代之的是,该结构(如kobject结构)通常嵌入在设备的更高级别表示中 */