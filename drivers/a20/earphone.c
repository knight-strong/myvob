#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/tty.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <mach/irqs.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/reboot.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <mach/sys_config.h>

#define PFX "EARPHONE "
#define EARPHONE_MAINKEY "earphone"
#define EARPHONE_INT_KEY "earphone_int"

static struct gpio_config gpio_int;

struct earphone_dev_t {
    struct class *sys_class;
    struct device *sys_device;
    dev_t devno;
    u32 cd_hdle;
    u8 state;
};

static struct earphone_dev_t *earphone_dev = NULL;

static void switch_amplifier(int state)
{
    // state: 0-off, 1-on
    struct gpio_config gio = {
        .gpio = GPIOG(11),   // todo: read from configure
        .mul_sel = GPIO_CFG_OUTPUT,
        .pull = 1,
        .drv_level = 1,
        .data = state
    };
    int r = gpio_request(gio.gpio, NULL);
    if (unlikely(r)){
        printk(KERN_ERR "ERROR: Gpio_request failed\n");
    }
    else {
        r = sw_gpio_setall_range(&gio, 1);
    }
    gpio_free(gio.gpio);
}

static irqreturn_t earphone_isr(void *dev_obj)
{
    struct earphone_dev_t *dev = (struct earphone_dev_t*)dev_obj;
    dev->state = __gpio_get_value(gpio_int.gpio);
    printk(KERN_INFO PFX "state: %d\n", dev->state);
    switch_amplifier(dev->state);
    return 0;
}

static ssize_t sysfs_show_state(struct device *drvdata, struct device_attribute *attr, char *buf)
{
    struct earphone_dev_t *dev = (struct earphone_dev_t*)dev_get_drvdata(drvdata);
    return sprintf(buf, "%d\n", dev->state);
}

static struct device_attribute sysfs_attrs[] = {
    __ATTR(state, S_IRUGO, sysfs_show_state, NULL),
    { },
};

static int sysfs_suspend(struct device *dev, pm_message_t mesg)
{
    printk(KERN_INFO PFX "sysfs_suspend\n");
    return 0;
}

static int sysfs_resume(struct device *dev)
{
    printk(KERN_INFO PFX "sysfs_resume\n");
    return 0;
}

struct earphone_dev_t* alloc_earphone_dev(void)
{
    struct earphone_dev_t *dev = (struct earphone_dev_t*)kmalloc(sizeof(*dev), GFP_KERNEL);
    memset(dev, 0, sizeof(*dev));

    dev->cd_hdle = sw_gpio_irq_request(gpio_int.gpio, 
            TRIG_EDGE_DOUBLE,
            &earphone_isr, dev);
    if (!dev->cd_hdle) {
        printk(KERN_ERR PFX "Failed to request gpio irq for earphone.\n");
        goto err;
    }

    return dev;
err:
    if (dev) {
        kfree(dev);
    }
    return NULL;
}

static int __init earphone_init(void)
{
    int ret;
    script_item_u   val;
    script_item_value_type_e  type;

    type = script_get_item(EARPHONE_MAINKEY, EARPHONE_INT_KEY, &val);
    if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
        printk(KERN_ERR PFX "fetch %s.%s failed\n", EARPHONE_MAINKEY, EARPHONE_INT_KEY);
#if 0
        return -1;
#else
        // default
        printk(KERN_INFO PFX "default configure: PH07\n");
        gpio_int.gpio = GPIOH(7);
        gpio_int.mul_sel = 6;
        gpio_int.pull = 0;
        gpio_int.drv_level = 0;
        gpio_int.data = 0;
#endif
    }
    else {
        memcpy(&gpio_int, &val.gpio, sizeof(struct gpio_config));
    }

    struct earphone_dev_t *dev = alloc_earphone_dev();
    if (dev == NULL) {
        printk(KERN_ERR PFX "ERROR: alloc_earphone_dev\n");
        return -1;
    }

    dev->sys_class = class_create(THIS_MODULE, "earphone");
    if (IS_ERR(dev->sys_class)) {
        printk(KERN_ERR PFX "%s: couldn't create class\n", __FILE__);
        return PTR_ERR(dev->sys_class);
    }

    dev->sys_class->suspend = sysfs_suspend;
    dev->sys_class->resume = sysfs_resume;
    dev->sys_class->dev_attrs = sysfs_attrs;

    ret = alloc_chrdev_region(&dev->devno, 0, 1, "earphone");
    if (ret < 0) {
        printk(KERN_ERR PFX "ERROR: alloc_chrdev_region");
    }

    dev->sys_device = device_create(dev->sys_class, NULL, dev->devno, dev, "earphone");
    if (IS_ERR(dev->sys_device)) {
        printk(KERN_ERR PFX "ERROR: device_create failed\n");
        return PTR_ERR(dev->sys_device);
    }

    dev->state = __gpio_get_value(gpio_int.gpio);
    earphone_dev = dev;

    printk(KERN_INFO PFX "init ok. state:%d\n", dev->state);
    return 0;
}

static void __exit earphone_exit(void)
{
    struct earphone_dev_t *dev = earphone_dev;

    if (dev != NULL) {
        sw_gpio_irq_free(dev->cd_hdle);
        unregister_chrdev_region(dev->devno, 1);
        device_destroy(dev->sys_class, dev->devno);
        class_destroy(dev->sys_class);
        kfree(dev);
    }
    gpio_free(gpio_int.gpio);
    earphone_dev = NULL;
}

module_init(earphone_init);
module_exit(earphone_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("hehj@routon.com");

