/* 
1. 确定主设备号，也可以让内核分配
2. 定义自己的 file_operations 结构体
3. 实现对应的 drv_open/drv_read/drv_write 等函数，填入 file_operations 结构体
4. 把 file_operations 结构体告诉内核：register_chrdev
5. 谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数
6. 有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用 unregister_chrdev
7. 其他完善：提供设备信息，自动创建设备节点：class_create,device_create*/
#include "asm/uaccess.h"
#include "linux/err.h"
#include "linux/export.h"
#include "linux/kdev_t.h"
#include "linux/moduleparam.h"
#include "linux/printk.h"
#include "linux/stddef.h"
#include "linux/usb/cdc.h"
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include "led_opr.h"


MODULE_AUTHOR("null-zzj zj.zhu.cn@gmail.com");
MODULE_DESCRIPTION("led drv");
MODULE_LICENSE("GPL");



#define MIN(a, b) (a < b ? a : b)

/*确定主设备号*/
static int major  = 0;
static struct class *led_class;
struct led_operations *p_led_opr;

static int LED_NUM;

// 3. 实现对应的 drv_open/drv_read/drv_write 等函数，填入 file_operations 结构体
static  int led_drv_open (struct inode *node, struct file *file)
{
    int minor;
    // 根据打开的次设备号初始对应的灯
    minor = iminor(node);
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    p_led_opr->init(minor);
    LED_NUM = get_board_led_num();
    return 0;
}

static ssize_t led_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
   
    int minor;
    minor = iminor(file_inode(file));
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
 
    return 0;
    
}

static ssize_t led_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int ret, minor;
    char status;
    minor = iminor(file_inode(file));
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    // 根据次设备号控制led
    ret = copy_from_user(&status, buf, 1);

    p_led_opr->ctl(minor, status);
    return ret;
}
static int led_drv_release (struct inode *node, struct file *file)
{
    int minor;
    minor = iminor(file_inode(file));
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    p_led_opr->release(minor);
    return 0;

}

/*定义自己的 file_operations 结构体*/
static struct file_operations led_drv = {
    .owner = THIS_MODULE,
    .open = led_drv_open,
    .read = led_drv_read,
    .write = led_drv_write,
    .release = led_drv_release,
};


// 4. 把 file_operations 结构体告诉内核：register_chrdev

// 5. 谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数
static int __init led_drv_init(void)
{
    int err;

    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    // 申请设备号，申请模块结构，插入模块结构
    major = register_chrdev(major, "100ask_led", &led_drv);

    // 创建设备文件
    led_class = class_create(THIS_MODULE, "100ask_led_class");
    err = PTR_ERR(led_class);
	if (IS_ERR(led_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "led");
		return -1;
	}

    
    device_create(led_class, NULL, MKDEV(major, 0), NULL, "100ask_led");
    
    
    p_led_opr = get_board_led_opr();

    return 0; 
}

// 6. 有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用 unregister_chrdev
static void __exit led_drv_exit(void)
{
    int i;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    for(i = 0; i < LED_NUM; i++)
    {
        device_destroy(led_class, MKDEV(major, i));
    }
    class_destroy(led_class);
    unregister_chrdev(0, "led"); 
}
// 7. 其他完善：提供设备信息，自动创建设备节点：class_create,device_create

module_init(led_drv_init);
module_exit(led_drv_exit);













