/* Linux
中断 按键GPIO触发 读阻塞
Reference:Alientek I.MX6U
 */

#include "io_block.h"

struct devstruct irqkey_dev;

static int irq_key_open(struct inode *inode, struct file *filp) //打开设备
{
    filp->private_data = &irqkey_dev;
    printk("open irq_key device in Linux!\r\n");
    return 0;
}

/* 应用程序调用read函数从设备中读取数据的时候此函数会执行
   buf：用户空间的内存
   cnt：要读取的字节数 
   return: 读取的字节数 为负读取失败*/
static ssize_t irq_key_read(struct file *filp, char __user *buf, //从设备读取
                            size_t cnt, loff_t *offt)
{
    int i;
    int retval;
    unsigned char key_info;
    unsigned char ok_flag;
    struct devstruct *dev = (struct devstruct *)filp->private_data;

    DECLARE_WAITQUEUE(wait, current); //宏，给当前进程创建一个叫wait的队列项

    for (i = 0; i < KEY_NUM; i++)
    {
        ok_flag = atomic_read(&dev->io_irq[i].ok_flag);
        if (ok_flag == 1)
        {
            atomic_add(1 << i, &dev->key_info);
        }
    }
    if (atomic_read(&dev->key_info) == 0)
    {
        add_wait_queue(&dev->r_wait, &wait);     //把进程添加到队列头中
        __set_current_state(TASK_INTERRUPTIBLE); //设置任务状态TASK_INTERRUPTIBLE 浅睡眠
        schedule();                              //任务调度(前面+printk会打印冲突，暂时不清楚原因,也不知道驱动层的printk如何强制输出）)
        printk("wake up babe\r\n");
        if (signal_pending(current)) //判断是否为信号引起的唤醒
        {
            retval = -ERESTARTSYS;
            goto wait_error;
        }
    }
    __set_current_state(TASK_RUNNING);      //设置为运行状态
    remove_wait_queue(&dev->r_wait, &wait); //移除等待队列

    //key_info 的每一位代表一个按键 0：按键0  1：按键1 ...
    for (i = 0; i < KEY_NUM; i++)
    {
        ok_flag = atomic_read(&dev->io_irq[i].ok_flag);
        if (ok_flag == 1)
        {
            atomic_add(1 << i, &dev->key_info);
            atomic_set(&dev->io_irq[i].ok_flag, 0);
        }
    }

    key_info = atomic_read(&dev->key_info);
    retval = __copy_to_user(buf, &key_info, sizeof(key_info));
    atomic_set(&dev->key_info, 0);
    return 0;

wait_error:
    printk("wait error babe\r\n");
    set_current_state(TASK_RUNNING);        //设置任务为运行态
    remove_wait_queue(&dev->r_wait, &wait); //将等待队列移除
    return retval;
}

/* 应用程序调用write函数从设备中读取数据的时候此函数会执行
   buf：应用程序要写入设备的数据
   cnt：要写入的字节数 
   return: 写入的字节数 为负写入失败*/
static ssize_t irq_key_write(struct file *filp, //向设备写入
                             const char __user *buf,
                             size_t cnt, loff_t *offt)
{
    printk(".write function is void!\r\n");
    return 0;
}

static int irq_key_release(struct inode *inode, struct file *filp)
{
    printk("Release irq_key device in Linux! \r\n");
    return 0;
}

static struct file_operations irq_key_fops = {
    .owner = THIS_MODULE,
    .open = irq_key_open,
    .read = irq_key_read,
    .write = irq_key_write,
    .release = irq_key_release,
};

//定时器回调函数
void irqkeyout_callback(unsigned long arg) //回调函数的参数就是irqkey_dev结构体的地址
{
    unsigned char key_value;
    struct devstruct *dev = (struct devstruct *)arg;

    key_value = gpio_get_value(dev->io_irq[dev->curkey_num].gpio); //根据当期按键号找到对应的IO结构体,并读取IO的值

    //按下和松开按键分别触发一次中断（抖动应该不会触发回调，会被按键中断的mod——timer反复刷新？）
    if (key_value == 0) //按键按下
    {
        atomic_set(&dev->io_irq[dev->curkey_num].value, 0);
    }

    else //按下后松开
    {
        atomic_set(&dev->io_irq[dev->curkey_num].value, 1);
        atomic_set(&dev->io_irq[dev->curkey_num].ok_flag, 1); //应用程序中的flag在这里置位
        wake_up_interruptible(&dev->r_wait);                  //唤醒进程
    }
}

/*中断服务函数*/
static irqreturn_t key0_handler(int irq, void *dev_id) //第一个参数是要处理的中断号，第二个参数貌似是requestirq时候传入的第二个参数，也就是设备结构体的地址（貌似就是第一个元素dev_id的地址）
{
    struct devstruct *dev = (struct devstruct *)dev_id;

    dev->curkey_num = 0; //储存当前按键号给timer统一处理
    dev->timer.data = (volatile unsigned long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    return IRQ_RETVAL(IRQ_HANDLED);
}

/*初始化设置*/
static int my_dev_init(void)
{
    uint8_t i = 0;
    int retval;

    for (i = 0; i < KEY_NUM; i++) //初始化节点
    {
        atomic_set(&irqkey_dev.io_irq[i].value, KEYVALUE);
        atomic_set(&irqkey_dev.io_irq[i].ok_flag, 0);
    }

    irqkey_dev.nd = of_find_node_by_path("/key"); //找到节点
    if (irqkey_dev.nd == NULL)
    {
        printk("key node not find!\r\n");
        return -EINVAL;
    }

    for (i = 0; i < KEY_NUM; i++)
    {
        irqkey_dev.io_irq[i].gpio = of_get_named_gpio(irqkey_dev.nd, "key-gpio", i); //获得gpio编号
        memset(irqkey_dev.io_irq[i].name, 0, sizeof(irqkey_dev.io_irq[i].name));
        sprintf(irqkey_dev.io_irq[i].name, "key%d", i); //定制化gpio名字

        gpio_request(irqkey_dev.io_irq[i].gpio, irqkey_dev.io_irq[i].name);
        gpio_direction_input(irqkey_dev.io_irq[i].gpio); //初始化GPIO

        printk("key%d, gpio number:%d, gpio name:%s\r\n",
               irqkey_dev.io_irq[i].gpio, irqkey_dev.io_irq[i].gpio,
               irqkey_dev.io_irq[i].name);
    }

    //现在只编写了按键0的中断服务函数，初始化前定义好
    irqkey_dev.io_irq[0].handler = key0_handler;

    for (i = 0; i < KEY_NUM; i++)
    {
        irqkey_dev.io_irq[i].irq_num = irq_of_parse_and_map(irqkey_dev.nd, i); //获得中断编号
        memset(irqkey_dev.io_irq[i].irqname, i, sizeof(irqkey_dev.io_irq[i].irqname));
        sprintf(irqkey_dev.io_irq[i].irqname, "keyirq%d", i); //定制化中断名字

        retval = request_irq(irqkey_dev.io_irq[i].irq_num,
                             irqkey_dev.io_irq[i].handler,
                             IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                             irqkey_dev.io_irq[i].irqname, &irqkey_dev);
        if (retval < 0)
        {
            printk("irq %d request failed!\r\n",
                   irqkey_dev.io_irq[i].irq_num);
            return -EFAULT;
        }

        printk("key%d, irq number:%d, irq name:%s\r\n",
               irqkey_dev.io_irq[i].gpio, irqkey_dev.io_irq[i].irq_num,
               irqkey_dev.io_irq[i].irqname);
    }

    /* 创建定时器 */
    init_timer(&irqkey_dev.timer);
    irqkey_dev.timer.function = irqkeyout_callback;
    //timer_function传入的参数在启动定时器前设定，可以随时打开定时器，设定周期和传入想要传入回调函数的参数

    //初始化等待队列头
    init_waitqueue_head(&irqkey_dev.r_wait);
    return 0;
}

static int __init irq_key_init(void) //驱动注册入口函数
{
    //设备号分配
    alloc_chrdev_region(&irqkey_dev.devid, 0, IRQKEY_COUNT, IRQKEY_NAME);
    irqkey_dev.majorid = MAJOR(irqkey_dev.devid);
    irqkey_dev.minorid = MINOR(irqkey_dev.devid);
    printk("major:%d minor:%d\r\n", irqkey_dev.majorid, irqkey_dev.minorid);

    //初始化设置，
    //linux内核提供初始化函数，需要初始化定时器，并指定回调函数 data传入了设备结构体的地址
    my_dev_init();

    //初始化cdev，向系统添加字符设备
    irqkey_dev.cdev.owner = THIS_MODULE;
    cdev_init(&irqkey_dev.cdev, &irq_key_fops);
    cdev_add(&irqkey_dev.cdev, irqkey_dev.devid, IRQKEY_COUNT); //用于向Linux系统注册设备

    //创建类以及利用mdev自动创建设备节点
    irqkey_dev.class = class_create(THIS_MODULE, IRQKEY_NAME);
    if (IS_ERR(irqkey_dev.class))
    {
        return PTR_ERR(irqkey_dev.class);
        printk("class error\r\n");
    }
    irqkey_dev.device = device_create(irqkey_dev.class, NULL, irqkey_dev.devid, NULL, IRQKEY_NAME);
    if (IS_ERR(irqkey_dev.device))
    {
        return PTR_ERR(irqkey_dev.device);
        printk("device error\r\n");
    }
    printk("irq_key driver register succeeded\r\n");

    return 0;
}

static void __exit irq_key_exit(void) //驱动注册出口函数
{
    int i;

    del_timer_sync(&irqkey_dev.timer); //删除定时器

    for (i = 0; i < KEY_NUM; i++)
    {
        free_irq(irqkey_dev.io_irq[i].irq_num, &irqkey_dev); //释放中断
    }

    cdev_del(&irqkey_dev.cdev);                               //注销设备
    unregister_chrdev_region(irqkey_dev.devid, IRQKEY_COUNT); //释放设备号
    device_destroy(irqkey_dev.class, irqkey_dev.devid);
    class_destroy(irqkey_dev.class); //删除类和设备

    printk("irq_key driver unregister succeeded\r\n");
}

module_init(irq_key_init); //注册 模块加载函数
module_exit(irq_key_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");
