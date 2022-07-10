/* Linux
INPUT子系统
驱动和设备匹配
Reference:Alientek I.MX6U
 */

#include "inputsys.h"

struct devstruct irqkey_dev;

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
        input_report_key(dev->inputdev, KEY_0, 1); //上传KEY0 0 给INPUT子系统，按下（规定按键的按下 value是1）
        input_sync(dev->inputdev);                 //同步用于告诉input core子系统报告结束
                                                   //如果报告的1一直没变0  应用层依然可以read，即使没有同步，read的value是2,意思是一直按下
    }

    else //按下后松开
    {
        atomic_set(&dev->io_irq[dev->curkey_num].value, 1);
        atomic_set(&dev->io_irq[dev->curkey_num].ok_flag, 1); //暂时无用
        input_report_key(dev->inputdev, KEY_0, 0);            //上传KEY0 1 给INPUT子系统，松开
        input_sync(dev->inputdev);
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
                             irqkey_dev.io_irq[i].irqname, &irqkey_dev);  //
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
    return 0;
}

static const struct of_device_id irq_key_of_match[] = { //Struct used for matching a device
    {.compatible = "atkalpha-key"},
    {}};

static int irq_key_probe(struct platform_device *dev)
{
    int retval;

    printk("led driver and device matched babe\r\n");

    my_dev_init();

    //input子系统，内部封装好了几乎所有的输入，定义input_dev的属性后，调用注册函数即可
    //之后就可以在驱动层按照规定的格式把信息发往核心层（这部分linux已经写好了），完成注册和与呈上启下交互等等
    //也不需要再关心文件操作接口，这部分由核心层和用户层之间的事件处理层解决
    //所以input子系统的驱动层只需要完成初始化和事件报告即可，用户层的（如read）都由事件层完成
    irqkey_dev.inputdev = input_allocate_device(); //申请input_dev
    irqkey_dev.inputdev->name = IRQKEY_NAME;
    __set_bit(EV_KEY, irqkey_dev.inputdev->evbit); //按键事件
    __set_bit(EV_REP, irqkey_dev.inputdev->evbit); //可以重复
    __set_bit(KEY_0, irqkey_dev.inputdev->keybit); //INPUT 按键值 设为KEY0(这个KEY0是内部定义好的按键值，不是我们的,只是注册一个标识，对应一个按键)

    //注册input_dev
    retval = input_register_device(irqkey_dev.inputdev);
    if (retval)
    {
        printk("register input device failed\r\n");
        return retval;
    }
    printk("irq_key driver register succeeded\r\n");
    return 0;
}

static int irq_key_remove(struct platform_device *dev)
{
    int i;

    del_timer_sync(&irqkey_dev.timer); //删除定时器

    for (i = 0; i < KEY_NUM; i++)
    {
        free_irq(irqkey_dev.io_irq[i].irq_num, &irqkey_dev); //释放中断
    }

    //注销input_dev
    input_unregister_device(irqkey_dev.inputdev);
    input_free_device(irqkey_dev.inputdev);

    printk("irq_key driver unregister succeeded\r\n");

    return 0;
}

//platform 驱动结构体
static struct platform_driver irq_key_driver = {
    .driver = {
        .name = "irq_key",                  //驱动名字（不用platform_device就不通过这个匹配了）
        .of_match_table = irq_key_of_match, //设备树匹配表（是通过compatible匹配了）
    },
    .probe = irq_key_probe,
    .remove = irq_key_remove,
};

static int __init irq_key_init(void) //驱动注册入口函数
{
    return platform_driver_register(&irq_key_driver);
}

static void __exit irq_key_exit(void) //驱动注册出口函数
{
    platform_driver_unregister(&irq_key_driver);
}

module_init(irq_key_init); //注册 模块加载函数
module_exit(irq_key_exit); //注册 模块卸载函数

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NYC");