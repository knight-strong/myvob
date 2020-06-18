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
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/reboot.h>

#define DEVNAME "dumy"
#define PFX "DUMY "

struct dumy_dev_t {
    struct class *sys_class;
    struct device *sys_device;
    dev_t devno;
    u8 state;
};

static struct dumy_dev_t *dumy_dev = NULL;

static ssize_t state_show(struct device *drvdata, struct device_attribute *attr, char *buf)
{
    struct dumy_dev_t *dev = (struct dumy_dev_t*)dev_get_drvdata(drvdata);
    return sprintf(buf, "%d\n", dev->state);
}

static ssize_t version_show(struct device *drvdata, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", "1.0");
}

static DEVICE_ATTR(state, S_IRUGO, state_show, NULL);
static DEVICE_ATTR(version, S_IRUGO, version_show, NULL);

static struct attribute *dumy_attrs[] = {
    &dev_attr_version.attr,
    &dev_attr_state.attr,
    NULL
};

static struct attribute_group dumy_attr_group = {
    .attrs = dumy_attrs,
};

static const struct attribute_group *dumy_attr_groups[] = {
    &dumy_attr_group,
    NULL
};

struct dumy_dev_t* alloc_dumy_dev(void)
{
    struct dumy_dev_t *dev = (struct dumy_dev_t*)kmalloc(sizeof(*dev), GFP_KERNEL);
    memset(dev, 0, sizeof(*dev));

    return dev;
}

static int __init dumy_init(void)
{
    int ret;
    char *envp[3] = {0};

    struct dumy_dev_t *dev = alloc_dumy_dev();
    if (dev == NULL) {
        printk(KERN_ERR PFX "ERROR: alloc_dumy_dev\n");
        return -1;
    }

    dev->sys_class = class_create(THIS_MODULE, DEVNAME);
    if (IS_ERR(dev->sys_class)) {
        printk(KERN_ERR PFX "%s: couldn't create class\n", __FILE__);
        return PTR_ERR(dev->sys_class);
    }

    ret = alloc_chrdev_region(&dev->devno, 0, 1, DEVNAME);
    if (ret < 0) {
        printk(KERN_ERR PFX "ERROR: alloc_chrdev_region");
    }

    dev->sys_device = device_create(dev->sys_class, NULL, dev->devno, dev, DEVNAME);
    if (IS_ERR(dev->sys_device)) {
        printk(KERN_ERR PFX "ERROR: device_create failed\n");
        return PTR_ERR(dev->sys_device);
    }
    device_add_groups(dev->sys_device, dumy_attr_groups);

    dumy_dev = dev;

    printk(KERN_INFO PFX "init ok. state:%d\n", dev->state);

    kobject_uevent_env(&dev->sys_device->kobj, KOBJ_ADD, envp);
    kobject_uevent_env(&dev->sys_device->kobj, KOBJ_OFFLINE, envp);
    return 0;
}

static void __exit dumy_exit(void)
{
    struct dumy_dev_t *dev = dumy_dev;

    if (dev != NULL) {
        kobject_uevent(&dev->sys_device->kobj, KOBJ_REMOVE);

        device_remove_groups(dev->sys_device, dumy_attr_groups);
        unregister_chrdev_region(dev->devno, 1);
        device_destroy(dev->sys_class, dev->devno);
        class_destroy(dev->sys_class);
        kfree(dev);
    }
    dumy_dev = NULL;
}

module_init(dumy_init);
module_exit(dumy_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("hehj@routon.com");

