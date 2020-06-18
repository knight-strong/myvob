#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include "gpio_sw.h"

#define DEBUG       1
#define PFX "watchdog: "

#define GPIO_WATCHDOG_EN        GPIOG(2)
#define GPIO_WATCHDOG_FEED      GPIOG(3)

#define WATCHDOG_TIMEOUT        30      /* seconds of default timeout */
#define CHECK_ALIVE_INTERVAL    10      /* check alive */
#define EXTERNAL_WATCHDOG_MINOR 133


static int watchdog_alive      = 0;
static spinlock_t watchdog_lock;	/* Spin lock */
static struct timer_list watchdog_ticktock;
static struct timer_list watchdog_10ticktock;
static int counter;
static int alive        =   1;
static int current_v    =   0;

static unsigned int watchdog_timeout = WATCHDOG_TIMEOUT;
static unsigned int init_timeout = 4 * WATCHDOG_TIMEOUT;

module_param(watchdog_timeout, int, 0);
module_param(init_timeout, int, 0);
MODULE_PARM_DESC(watchdog_timeout, "Watchdog timeout (in seconds)");
MODULE_PARM_DESC(init_timeout, "Watchdog timeout for init (in seconds)");

static int gpio_cfg_out(int gpio, int data)
{
    struct gpio_config gio = {
        .gpio = gpio,
        .mul_sel = GPIO_CFG_OUTPUT,
        .pull = 1,
        .drv_level = 1,
        .data = data
    };
    int r = sw_gpio_setall_range(&gio, 1);
    return r;
}

extern void set_kernel_stop_feeddog(void);

static void watchdog_start(void)
{
    printk(KERN_INFO PFX "start.\n");
    // set 1 to alive normal mode
    alive = 1;
}

static void watchdog_stop(void)
{
    printk(KERN_INFO PFX "stop.\n");
    // set 2 to alive always
    alive = 2;
}

static void watchdog_ping(void)
{
    counter = watchdog_timeout;
}

static void watchdog_fire(unsigned long data)
{
    if (alive) {
#if 1
        // printk(KERN_DEBUG "fire\n");
        current_v = !current_v;
        __gpio_set_value(GPIO_WATCHDOG_FEED, current_v);
#endif
        mod_timer(&watchdog_ticktock, jiffies+(HZ>>1));
    }
    else {
        printk(KERN_DEBUG "!!! NOT alive now, stop feed !!!\n");
    }
}

static void watchdog_check_alive(unsigned long data)
{
    mod_timer(&watchdog_10ticktock, jiffies+(CHECK_ALIVE_INTERVAL*HZ));

    spin_lock(&watchdog_lock);
    if (alive == 1) {
        if ((counter -= CHECK_ALIVE_INTERVAL) <= 0)
            alive = 0;
    }
    spin_unlock(&watchdog_lock);

    // printk(KERN_DEBUG "watchdog_check... %d, %d\n", counter, alive);
}

/*
 *	Allow only one person to hold it open
 */
static int watchdog_open(struct inode *inode, struct file *file)
{
    spin_lock(&watchdog_lock);

    if (watchdog_alive) {
        spin_unlock(&watchdog_lock);
        return -EBUSY;
    }

    watchdog_alive = 1;

    spin_unlock(&watchdog_lock);

    return nonseekable_open(inode, file);
}

static int watchdog_release(struct inode *inode, struct file *file)
{
	spin_lock(&watchdog_lock);
	watchdog_alive = 0;
	spin_unlock(&watchdog_lock);
	return 0;
}

static ssize_t watchdog_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
	spin_lock(&watchdog_lock);
	if (len) 
		watchdog_ping();
	spin_unlock(&watchdog_lock);
	return len;
}

static long watchdog_unlocked_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	int options, retval = -EINVAL;
	static struct watchdog_info ident = {
		.options		= WDIOF_KEEPALIVEPING |
					  WDIOF_MAGICCLOSE,
		.firmware_version	= 0,
		.identity		= "Hardware e-Watchdog",
	};

	spin_lock(&watchdog_lock);
	switch (cmd) {
		case WDIOC_GETSUPPORT:
			if (copy_to_user((struct watchdog_info *)arg, &ident, sizeof(ident))) 
				retval = -EFAULT;
			else
				retval = 0;
			break;
		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			retval = put_user(0, (int *)arg);
			break;
		case WDIOC_KEEPALIVE:
			watchdog_ping();
			retval = 0;
			break;
		case WDIOC_GETTIMEOUT:
			retval = put_user(watchdog_timeout, (int *)arg);
			break;
		case WDIOC_SETTIMEOUT:
			retval = get_user(watchdog_timeout, (int *)arg);
                        watchdog_ping();
			break;
		case WDIOC_SETOPTIONS:
		{
			if (get_user(options, (int *)arg)) {
				retval = -EFAULT;
				break;
			}

			if (options & WDIOS_DISABLECARD) {
				watchdog_stop();
				retval = 0;
			}

			if (options & WDIOS_ENABLECARD) {
				watchdog_start();
                                watchdog_ping();
				retval = 0;
			}

			break;
		}
		default:
			retval = -ENOIOCTLCMD;
			break;
	}
	spin_unlock(&watchdog_lock);

	return retval;
}

static int watchdog_notify_sys(struct notifier_block *this, unsigned long code, void *unused)
{
#if 0
	if (code == SYS_DOWN || code == SYS_HALT)
		watchdog_stop();
#endif
        printk(KERN_INFO PFX "notifier: reboot\n");
	return NOTIFY_DONE;
}

static struct file_operations watchdog_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.write		= watchdog_write,
	.unlocked_ioctl	= watchdog_unlocked_ioctl,
	.open		= watchdog_open,
	.release	= watchdog_release,
};

static struct miscdevice watchdog_miscdev = {
	.minor		= EXTERNAL_WATCHDOG_MINOR,
	.name		= "e-watchdog",
	.fops		= &watchdog_fops,
};

static struct notifier_block watchdog_notifier = {
	.notifier_call = watchdog_notify_sys,
};

static char banner[] __initdata = KERN_INFO PFX "Hardware e-Watchdog (def. timeout: %d sec)\n";

static int __init watchdog_init(void)
{
	int ret;

	ret = register_reboot_notifier(&watchdog_notifier);
	if (ret) {
		printk(KERN_ERR PFX "cannot register reboot notifier (err=%d)\n", ret);
		return ret;
	}

	ret = misc_register(&watchdog_miscdev);
	if (ret) {
		printk(KERN_ERR PFX "cannot register miscdev on minor=%d (err=%d)\n", watchdog_miscdev.minor, ret);
		unregister_reboot_notifier(&watchdog_notifier);
		return ret;
	}

	printk(banner, watchdog_timeout);

        spin_lock_init(&watchdog_lock);

        {
            // start watchdog, 首次允许喂狗时长
            counter = init_timeout;
            printk(KERN_INFO "Started e-watchdog timer. init_timeout=%d\n", counter);

            /* enabled e-watchdog, out 0 */
            gpio_cfg_out(GPIO_WATCHDOG_EN, 0);
            gpio_cfg_out(GPIO_WATCHDOG_FEED, 1);

            current_v = !current_v;
            gpio_cfg_out(GPIO_WATCHDOG_FEED, current_v);

            init_timer(&watchdog_ticktock);
            watchdog_ticktock.function = watchdog_fire;
            mod_timer(&watchdog_ticktock, jiffies+(HZ>>1));

            init_timer(&watchdog_10ticktock);
            watchdog_10ticktock.function = watchdog_check_alive;
            mod_timer(&watchdog_10ticktock, jiffies+(CHECK_ALIVE_INTERVAL*HZ));
        }

	return 0;
}

static void __exit watchdog_exit(void)
{
        del_timer(&watchdog_ticktock);
        del_timer(&watchdog_10ticktock);

	misc_deregister(&watchdog_miscdev);
	unregister_reboot_notifier(&watchdog_notifier);

        printk(KERN_INFO PFX "exit.\n");
}

module_init(watchdog_init);
module_exit(watchdog_exit);

MODULE_DESCRIPTION("External Hardware Watchdog Device");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
MODULE_ALIAS_MISCDEV(EXTERNAL_WATCHDOG_MINOR);
MODULE_AUTHOR("hehaojun");

