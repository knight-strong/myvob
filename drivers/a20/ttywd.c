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


#define WD_TTY_NAME     "/dev/ttyS0"
#define PERR printk

static struct file * f;
static struct tty_struct *tty;


static long tty_ioctl(struct file *f, unsigned op, unsigned long param)
{
    if (f->f_op->unlocked_ioctl)
        return f->f_op->unlocked_ioctl(f, op, param);

    return -ENOSYS;
}

static int tty_write(struct file *f, unsigned char *buf, int count)
{
    int result;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    f->f_pos = 0;
    result = f->f_op->write(f, buf, count, &f->f_pos);
    set_fs(oldfs);
    return result;
}

static int tty_read(struct file *f, unsigned char *buf, int count)
{
    int result;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    // f->f_pos = 0;
    result = f->f_op->read(f, buf, count, &f->f_pos);
    set_fs(oldfs);
    return result;
}

static int __init ttywd_init(void)
{
    mm_segment_t oldfs;

    f = filp_open("/dev/ttyS0", O_RDWR | O_NDELAY, 0); // , S_IREAD | S_IWRITE);
    if (f < 0) {
        PERR("open %s failed.\n", WD_TTY_NAME);
        return -1;
    }

    oldfs = get_fs(); 
    set_fs(KERNEL_DS); 
    {
        tty = (struct tty_struct*)f->private_data;

        /*  Set speed */
        struct termios settings;
        settings.c_iflag = 0;
        settings.c_oflag = 0;
        settings.c_lflag = 0;
        settings.c_cflag = CLOCAL | CS8 | CREAD;
        settings.c_cc[VMIN] = 0;
        settings.c_cc[VTIME] = 0;
        settings.c_cflag |= B115200;
        tty_ioctl(f, TCSETS, (unsigned long)&settings);

        {
            /*  Set low latency */
            struct serial_struct settings;
            tty_ioctl(f, TIOCGSERIAL, (unsigned long)&settings);
            settings.flags |= ASYNC_LOW_LATENCY;
            tty_ioctl(f, TIOCSSERIAL, (unsigned long)&settings);
        }
    }
    set_fs(oldfs);

    tty_write(f, "hello", 6);
    return 0;
}

static void __exit ttywd_exit(void)
{
    return;
}

module_init(ttywd_init);
module_exit(ttywd_exit);

MODULE_DESCRIPTION("External Hardware Watchdog for TTY");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("hehaojun");

