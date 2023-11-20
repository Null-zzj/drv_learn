#include "asm-generic/errno.h"
#include "asm/signal.h"
#include "linux/jiffies.h"
#include "linux/ktime.h"
#include "linux/timekeeping.h"
#include "linux/wait.h"
#include <asm-generic/gpio.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/timer.h>
#include <linux/tty.h>

MODULE_LICENSE("GPL");
#define CMD_TRIG 100

struct gpio_desc
{
    int gpio;
    int irq;
    char *name;
    int key;
    struct timer_list key_timer;
};

static struct gpio_desc gpios[2] = {
    {
        115,
        0,
        "trig",
    },
    {
        116,
        0,
        "echo",
    },

};

/* 主设备号      */
static int major = 0;
static struct class *gpio_class;

/* 环形缓冲区 */
#define BUF_LEN 128
static int g_vals[BUF_LEN];
static int r, w;

struct fasync_struct *button_fasync;

#define NEXT_POS(x) ((x + 1) % BUF_LEN)

static int is_val_buf_empty(void)
{
    return (r == w);
}

static int is_cal_buf_full(void)
{
    return (r == NEXT_POS(w));
}

static void put_val(int key)
{
    if (!is_cal_buf_full())
    {
        g_vals[w] = key;
        w = NEXT_POS(w);
    }
}

static int get_val(void)
{
    int val = 0;
    if (!is_val_buf_empty())
    {
        val = g_vals[r];
        r = NEXT_POS(r);
    }
    return val;
}



static DECLARE_WAIT_QUEUE_HEAD(gpio_wait);

// 定时器超时函数
static void sr04_timer_func(unsigned long data)  
{
    put_val(-1);
    wake_up_interruptible(&gpio_wait);
    kill_fasync(&button_fasync, SIGIO, POLL_IN);



}

/* 实现对应的open/read/write等函数，填入file_operations结构体                   */
static ssize_t sr04_drv_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    // printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    int err;
    int val;

    if (is_val_buf_empty() && (file->f_flags & O_NONBLOCK))
        return -EAGAIN;

    wait_event_interruptible(gpio_wait, !is_val_buf_empty());
    val = get_val();
    if(val == -1)
        return -ENODATA;

    err = copy_to_user(buf, &val, 4);

    return 4;
}

// sys_polld调用
static unsigned int sr04_drv_poll(struct file *fp, poll_table *wait)
{
    // printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    poll_wait(fp, &gpio_wait, wait);
    return is_val_buf_empty() ? 0 : POLLIN | POLLRDNORM; // 返回0 sys_pll 休眠一段时间继续调用
}

static int sr04_drv_fasync(int fd, struct file *file, int on)
{
    if (fasync_helper(fd, file, on, &button_fasync) >= 0)
        return 0;
    else
        return -EIO;
}

// ioctl (fd, cmd, arg)
long sr04_drv_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
    case CMD_TRIG: {
        gpio_set_value(gpios[0].gpio, 1);
        udelay(20);
        gpio_set_value(gpios[0].gpio, 0);


        // start timer
        mod_timer(&gpios[1].key_timer, jiffies + msecs_to_jiffies(50));
    }
    }
    return 0;
}

/* 定义自己的file_operations结构体                                              */
static struct file_operations gpio_sr04_drv = {
    .owner = THIS_MODULE,
    .read = sr04_drv_read,
    .poll = sr04_drv_poll,
    .fasync = sr04_drv_fasync,
    .unlocked_ioctl = sr04_drv_ioctl,
};

static irqreturn_t gpio_sr04_isr(int irq, void *dev_id)
{
    struct gpio_desc *gpio_desc = dev_id;
    /* data ==> gpio */
    int val;

    static u64 rising_time = 0;
    u64 time;

    val = gpio_get_value(gpio_desc->gpio);
    if (val)
    {
        // 上升沿记录起始时间
        rising_time = ktime_get_ns();
    }
    else
    {
        if (rising_time == 0)
        {
            printk("missing raising interrupt");
            return IRQ_HANDLED;
        }
        // 下降沿记录结束时间，并计算时间差 计算距离

        // stop timer
        del_timer(&gpios[1].key_timer);
        time = ktime_get_ns() - rising_time;
        rising_time = 0;
        put_val(time);
        wake_up_interruptible(&gpio_wait);           // 中断
        kill_fasync(&button_fasync, SIGIO, POLL_IN); //  异步
    }

    // printk("key_timer_expire key %d %d\n", gpio_desc->gpio, val);

    return IRQ_HANDLED;
}

/* 在入口函数 */
static int __init gpio_drv_init(void)
{
    int err;
    // trig pin
    err = gpio_request(gpios[0].gpio, gpios[0].name);
    if (err)
    {
        printk("%s: can not open GPIO %d\n", __func__, gpios[0].gpio);
        return 0;
    }
    err = gpio_direction_output(gpios[0].gpio, 0);
    if (err)
    {
        pr_err("Failed to direction input the GPIO %d\n", gpios[1].gpio);
        return 0;
    }

    // echo pin
    err = gpio_request(gpios[1].gpio, gpios[1].name);
    if (err)
    {
        printk("%s: can not open GPIO %d\n", __func__, gpios[0].gpio);
        return 0;
    }
    // 将echo引脚注册到中断号
    gpios[1].irq = gpio_to_irq(gpios[1].gpio);
    err =
        request_irq(gpios[1].irq, gpio_sr04_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, gpios[1].name, &gpios[1]);

    setup_timer(&gpios[1].key_timer, sr04_timer_func, (unsigned long)&gpios[1]);
    /* 注册file_operations 	*/
    major = register_chrdev(0, "100ask_gpio_sr04", &gpio_sr04_drv); /* /dev/gpio_desc */

    gpio_class = class_create(THIS_MODULE, "100ask_gpio_key_class");
    if (IS_ERR(gpio_class))
    {
        printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
        unregister_chrdev(major, "100ask_gpio_sr04");
        return PTR_ERR(gpio_class);
    }

    device_create(gpio_class, NULL, MKDEV(major, 0), NULL, "sr04"); /* /dev/100ask_gpio */

    return err;
}

/* 有入口函数就应该有出口函数：卸载驱动程序时，就会去调用这个出口函数
 */
static void __exit gpio_drv_exit(void)
{
    printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);

    device_destroy(gpio_class, MKDEV(major, 0));
    class_destroy(gpio_class);
    unregister_chrdev(major, "100ask_gpio_sr04");

    // 释放gpio口与与其绑定的中断
    gpio_free(gpios[0].gpio);
    gpio_free(gpios[1].gpio);
    free_irq(gpios[1].irq, &gpios[1]);
    del_timer(&gpios[1].key_timer);
}

/* 7. 其他完善：提供设备信息，自动创建设备节点                                     */

module_init(gpio_drv_init);
module_exit(gpio_drv_exit);
