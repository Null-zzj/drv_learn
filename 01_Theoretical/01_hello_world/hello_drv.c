/* 
1. 确定主设备号，也可以让内核分配
2. 定义自己的 file_operations 结构体
3. 实现对应的 drv_open/drv_read/drv_write 等函数，填入 file_operations 结构体
4. 把 file_operations 结构体告诉内核：register_chrdev
5. 谁来注册驱动程序啊？得有一个入口函数：安装驱动程序时，就会去调用这个入口函数
6. 有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用 unregister_chrdev
7. 其他完善：提供设备信息，自动创建设备节点：class_create,device_create*/
#include "asm-generic/errno-base.h"
#include "linux/cdev.h"
#include "linux/kern_levels.h"
#include <asm/uaccess.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/kdev_t.h>
#include <linux/moduleparam.h>
#include <linux/printk.h>
#include <linux/stddef.h>
#include <linux/usb/cdc.h>
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

#define MIN(a, b) (a < b ? a : b)

static struct cdev hello_drv;
static dev_t dev; 

// 1. 确定主设备号 0自动分配
static int major = 0;
static char kernel_buf[1024];
MODULE_LICENSE("GPL");

struct class* hello_class;


// 3. 填充file_operations
int hello_open (struct inode *node, struct file *file)
{
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}
ssize_t hello_read (struct file *file, char __user *buf, size_t size, loff_t *l)
{
    int ret;
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    ret = copy_to_user(buf, kernel_buf, MIN(1024, size));
    return ret;
}
ssize_t hello_write (struct file *file, const char __user *buf, size_t size, loff_t *l)
{
    int ret;
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    ret = copy_from_user(kernel_buf, buf, MIN(1024, size));
    return ret;
}
int hello_close (struct inode *node, struct file *file)
{
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    return 0;
}

// 2. 定义自己的 file_operations 结构体

struct file_operations operations= {
    .read = hello_read,
    .open = hello_open,
    .write = hello_write,
    .release = hello_close,
};

static int __init hello_drv_init(void)
{
    int err;
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

#define REGISETR_CHRDEV
#ifndef REGISETR_CHRDEV
// 4. 申请设备号，并将模块添加到内核中
    major = register_chrdev(major, "hello", &operations);
#else 
    
    err = alloc_chrdev_region(&dev, 0, 1, "hello");
    major = MAJOR(dev);
    if(err < 0)
    {
        printk(KERN_ERR "alloc_chrdev_region() faild for hello");
        return -EINVAL;
    }
    cdev_init(&hello_drv, &operations);
    err = cdev_add(&hello_drv, major, 1);
    if (err < 0)
		return err;
    
#endif

    // 创建设备文件
    hello_class = class_create(THIS_MODULE, "hello_class");
    err = PTR_ERR(hello_class);
	if (IS_ERR(hello_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "hello");
		return -1;
	}

    device_create(hello_class, NULL, dev, NULL, "hello");

    return 0; 
}

// 6. 有入口函数就应该有出口函数：卸载驱动程序时，出口函数调用 unregister_chrdev
static void __exit hello_drv_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    device_destroy(hello_class, dev);
    class_destroy(hello_class);

#ifndef REGISETR_CHRDEV
    unregister_chrdev(major, "hello");
#else 
    cdev_del(&hello_drv);
    unregister_chrdev_region(MAJOR(dev), 1);
    
#endif
  
}
// 7. 其他完善：提供设备信息，自动创建设备节点：class_create,device_create

module_init(hello_drv_init);
module_exit(hello_drv_exit);













