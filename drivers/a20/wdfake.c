#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <linux/device.h>
#include <linux/serial.h>
#include <linux/export.h>
#include <asm/termbits.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/termios.h>
#include <asm/ioctls.h>
#include <linux/serial.h>
#include <linux/poll.h>

#define PERR printk

static int __init wdfake_init(void)
{
    return 0;
}

static void __exit wdfake_exit(void)
{
    return;
}

module_init(wdfake_init);
module_exit(wdfake_exit);

MODULE_DESCRIPTION("Watchdog interface");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("hehj@routon.com");

