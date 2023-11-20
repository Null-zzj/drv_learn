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

/*
ccm使能gpio5  base 0x20c4006 [31:30] 0b11
设置gpio5复用功能mux  0x229001C [3:0] 0b0101
设置gpio的方向为output GPIO direction register  20A_C004 [5] 0b1

设置输出高低电平 0x20A_C000 [3] 1 或者 0
*/

static volatile int* CCM_GPOI5;
static volatile int* MUX_GPOI5;
static volatile int* DIRE_GPOI5;
static volatile int* DATA_GPOI5;



int led_board_init (int which)
{
    
    CCM_GPOI5 = ioremap(0x20c4006, 4); 
    MUX_GPOI5 = ioremap(0x2290014, 4);
    DIRE_GPOI5 = ioremap(0x20AC004, 4);
    DATA_GPOI5 = ioremap(0x20AC000, 4);

    *CCM_GPOI5 |= 0x3 << 30;
    *MUX_GPOI5 &= ~0xf;
    *MUX_GPOI5 |= 0x5;
    *DIRE_GPOI5 |= 1 << 3;

    *DATA_GPOI5 |= 1 << 3;


    return 0;
}


int led_board_ctl (int which, char status)
{
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
    if (status == 0)
    {
        *DATA_GPOI5 &= ~(1 << 3);
    }
    else if(status == 1)
    {
        *DATA_GPOI5 |= 1 << 3;
    }
    return 0;
}
int led_board_read( int which, char* statuc)
{
    printk("%s %s line %d", __FILE__, __FUNCTION__, __LINE__);
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



struct led_operations led_opr = {
    .init = led_board_init,
    .ctl = led_board_ctl,
    .read = led_board_read,
    .release = led_board_release,
};

struct led_operations* get_board_led_opr(void)
{
    
    return &led_opr;
}


int get_board_led_num(void)
{
    return 1;
}
