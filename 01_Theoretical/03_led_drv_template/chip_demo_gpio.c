#include "asm/io.h"
#include "led_opr.h"
#include "asm/uaccess.h"
#include "linux/cdev.h"
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
#include "led_resource.h"
// struct led_operations{
//     int (*init) (int which);  // 根据次设备号初始化led
//     int (*ctl) (int which, char status);
//     int (*read) (int which, char* status);
//     int (*release) (int which);
// };

static volatile int* CCM_GPOI5;
static volatile int* MUX_GPOI5;
static volatile int* DIRE_GPOI5;
static volatile int* DATA_GPOI5;


static struct led_resource *led_rsc;

int led_board_init (int which)
{
    if(!led_rsc)
    {
        led_rsc = get_led_resources();
    }

    switch (which) {
        case 1: 
            CCM_GPOI5 = ioremap(0x20c4006, 4); 
            MUX_GPOI5 = ioremap(0x2290014, 4);
            DIRE_GPOI5 = ioremap(0x20AC004, 4);
            DATA_GPOI5 = ioremap(0x20AC000, 4);

            *CCM_GPOI5 |= 0x3 << 30;
            *MUX_GPOI5 &= ~0xf;
            *MUX_GPOI5 |= 0x5;
            *DIRE_GPOI5 |= 1 << 3;

            *DATA_GPOI5 |= 0 << 3; // 默认开灯
            break;
        case 2:
            break;
        default:
            break;
    }

    return 0;
}


int led_board_ctl (int which, char status)
{
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    switch (which)
    {
        case 0:
            if (status == 1)
            {
                *DATA_GPOI5 &= ~(1 << 3);
            }
            else if(status == 0)
            {
                *DATA_GPOI5 |= 1 << 3;
            }
        break;
    case 1:
        break;
    default:
        break;
    }
    return 0;
}

int led_board_release(int which)
{
    iounmap(CCM_GPOI5);
    iounmap(MUX_GPOI5);
    iounmap(DIRE_GPOI5);
    iounmap(DATA_GPOI5);
    return 0;
}




