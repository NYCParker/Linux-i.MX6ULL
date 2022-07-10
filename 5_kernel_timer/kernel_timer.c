/* Linux
Kernel Timer 内核定时器
Reference:Alientek I.MX6U
 */

#include "kernel_timer.h"

struct devstruct timer_dev;

static int kernel_timer_open(struct inode *inode, struct file *filp) //打开设备
{
    filp->private_data = &timer_dev;
    timer_dev.timer_period = 1000;
    printk("open kernel_timer device in Linux!\r\n");
    printk("timer initial period = %d\r\n", timer_dev.timer_period);
    return 0;
}

/* 应用程序调用read函数从设备中读取数据的时候此函数会执行
   buf：用户空间的内存
   cnt：要读取的字节数 
   return: 读取的字节数 为负读取失败*/
static ssize_t kernel_timer_read(struct file *filp, char __user *buf, //从设备读取
                                 size_t cnt, loff_t *offt)
{
    printk(".read function is void!\r\n");
    return 0;
}

/* 应用程序调用write函数从设备中读取数据的时候此函数会执行
   buf：应用程序要写入设备的数据
   cnt：要写入的字节数 
   return: 写入的字节数 为负写入失败*/
static ssize_t kernel_timer_write(struct file *filp, //向设备写入
                                  const char __user *buf,
                                  size_t cnt, loff_t *offt)
{
    printk(".write function is void!\r\n");
    return 0;
}

static int kernel_timer_release(struct inode *inode, struct file *filp)
{
    printk("Release kernel_timer device in Linux! \r\n");
    return 0;
}

/*
* @description : ioctl 函数，
* @param – filp : 要打开的设备文件(文件描述符)
* @param - cmd : 应用程序发送过来的命令
* @param - arg : 参数   (比如TIMER 命令cmd是修改定时器周期 arg是要传入的修改的周期值)
* @return : 0 成功;其他 失败
*/
static long kernel_timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct devstruct *dev = (struct devstruct *)filp->private_data;
    int period;
    unsigned long flags; //中断状态（自旋锁相关）

    switch (cmd)
    {
    case CLOSE_CMD:
        del_timer_sync(&dev->timer); //删除一个定时器（会先等待定时处理器函数退出）
        printk("delete a timer\r\n");
        break;
    case OPEN_CMD:
        spin_lock_irqsave(&dev->lock, flags); //在进程中获取自旋锁，保存中断状态
        period = dev->timer_period;
        spin_unlock_irqrestore(&dev->lock, flags);                                    //释放自旋锁，恢复中断状态
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(period));                   //mod_timer 函数用于修改定时值，如果定时器还没有激活的话， mod_timer 函数会激活定时器 break;
        printk("start a timer, timer initial period = %d\r\n", timer_dev.timer_period); //int mod_timer(struct timer_list *timer, unsigned long expires) expires：修改后的超时时间。
        break;
    case SETPERIOD_CMD:
        spin_lock_irqsave(&dev->lock, flags);
        dev->timer_period = arg;
        period = arg;
        spin_unlock_irqrestore(&dev->lock, flags);
        mod_timer(&dev->timer, jiffies + msecs_to_jiffies(period));
        printk("change timer period, timer period = %d\r\n", timer_dev.timer_period);
        break;
    default:
        break;
    }

    return 0;
}

static struct file_operations kernel_timer_fops = {
    .owner = THIS_MODULE,
    .open = kernel_timer_open,
    .read = kernel_timer_read,
    .write = kernel_timer_write,
    .release = kernel_timer_release,
    .unlocked_ioctl = kernel_timer_unlocked_ioctl,
};

//定时器回调函数
void timerout_callback(unsigned long arg)  //回调函数的参数就是timer_dev结构体的地址
{
    struct devstruct *dev = (struct devstruct *)arg;
    //printk("surprise babe\r\n");

    //重启定时器
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timer_period));
}


static int __init kernel_timer_init(void) //驱动注册入口函数
{
    spin_lock_init(&timer_dev.lock);

    //设备号分配
    alloc_chrdev_region(&timer_dev.devid, 0, TIMER_COUNT, TIMER_NAME);
    timer_dev.majorid = MAJOR(timer_dev.devid);
    timer_dev.minorid = MINOR(timer_dev.devid);
    printk("major:%d minor:%d\r\n", timer_dev.majorid, timer_dev.minorid);

    //初始化cdev，向系统添加字符设备
    timer_dev.cdev.owner = THIS_MODULE;
    cdev_init(&timer_dev.cdev, &kernel_timer_fops);
    cdev_add(&timer_dev.cdev, timer_dev.devid, TIMER_COUNT); //用于向Linux系统注册设备

    //创建类以及利用mdev自动创建设备节点
    timer_dev.class = class_create(THIS_MODULE, TIMER_NAME);

    if (IS_ERR(timer_dev.class))
    {
        return PTR_ERR(timer_dev.class);
        printk("class error\r\n");
    }

    timer_dev.device = device_create(timer_dev.class, NULL, timer_dev.devid, NULL, TIMER_NAME);
    if (IS_ERR(timer_dev.device))
    {
        return PTR_ERR(timer_dev.device);
        printk("device error\r\n");
    }
    printk("kernel_timer driver register succeeded\r\n");

    //初始化定时器***  相当于注册定时器设备的时候，linux内核提供初始化函数，需要初始化定时器，并指定回调函数 data传入了设备结构体的地址
    init_timer(&timer_dev.timer);
    timer_dev.timer.function = timerout_callback;
    timer_dev.timer.data = (unsigned long)&timer_dev;

    return 0;
}

static void __exit kernel_timer_exit(void) //驱动注册出口函数
{
    cdev_del(&timer_dev.cdev);                            //注销设备
    unregister_chrdev_region(timer_dev.devid, TIMER_COUNT); //释放设备号

    device_destroy(timer_dev.class, timer_dev.devid);
    class_destroy(timer_dev.class); //删除类和设备
    printk("kernel_timer driver unregister succeeded\r\n");
}

module_init(kernel_timer_init); //注册 模块加载函数
module_exit(kernel_timer_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");

//cdev和device的区别
/* 在最低级别上,Linux系统中的每个设备都由struct device实例表示
设备结构包含设备模型核心对系统进行建模所需的信息
但是,大多数子系统会跟踪有关其托管设备的其他信息.结果,很少有设备用裸设备结构来表示.
取而代之的是,该结构(如kobject结构)通常嵌入在设备的更高级别表示中 */