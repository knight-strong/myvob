#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/wait.h>

#define DRV_VERSION     "1.0"

MODULE_AUTHOR("hehaojun <hehj@routon.com>");
MODULE_DESCRIPTION("hehj test driver");
MODULE_VERSION(DRV_VERSION);
MODULE_LICENSE("GPL");
MODULE_ALIAS("testdev");

struct testdev {
    struct timer_list timer;
    wait_queue_head_t waitq;
    struct wait_queue_entry wait;
    int read_ready;

    struct workqueue_struct *wq;
    struct work_struct work;
    struct delayed_work delaywork;

    struct tasklet_struct task;
    spinlock_t lock;

    struct kobject *kobj;
    struct kset *ks;
};

static struct testdev tdev;


static ssize_t show_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", 123);
}

static ssize_t store_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    printk("data: %s", buf);
    return count;
}

static DEVICE_ATTR(reg, S_IWUSR | S_IRUGO, show_reg, store_reg);

static struct attribute *board_sysfs_entries[] = { 
    &dev_attr_reg.attr,
    NULL
};

static struct attribute_group system_attribute_group = { 
        .name = NULL,
        .attrs = board_sysfs_entries,
};


static int create_sysfs(void)
{
    int retval;

    tdev.kobj = kobject_create_and_add("testdev", kernel_kobj);

    if (!tdev.kobj)
        return -ENOMEM;

    tdev.ks = kset_create_and_add("kset_tdev", NULL, kernel_kobj);
    if (!tdev.ks)
        return -ENOMEM;

    retval = sysfs_create_group(tdev.kobj, &system_attribute_group);
    if (retval) {
        printk(KERN_ERR "sysfs_create_group() FAILED\n");
    }

    printk(KERN_INFO "%s() l %d, create sysfs done\n", __func__, __LINE__);
    return retval;
}

static void release_sysfs(void)
{
    sysfs_remove_group(tdev.kobj, &system_attribute_group);
    kobject_put(tdev.kobj);
    kset_unregister(tdev.ks);
}

void mytimer_fn(struct timer_list * argv)
{
    printk(KERN_INFO "%s() l %d, my jiffies:%ld\n", __func__, __LINE__, tdev.timer.expires);
    tdev.read_ready = 1;
    wake_up_interruptible(&tdev.waitq);
#if 0
    mod_timer(&tdev.timer, jiffies + HZ);
    static int n = 0;
    if (tdev.read_ready == 0) {
        if (n++ < 3) {
            printk(KERN_INFO "%s() l %d, wakeup ... %d \n", __func__, __LINE__, n);
            tdev.read_ready = 1;
            wake_up_interruptible(&tdev.waitq);
        }
    }
#endif
}

static void create_timer(void)
{
    struct timer_list *t = &tdev.timer;
    __init_timer(t, mytimer_fn, 0);
    // init_timer(t);
    //t->function = mytimer_fn;

    mod_timer(t, jiffies + HZ);
    // t->expires = jiffies + HZ;
    // add_timer(t);
}

static void release_timer(void)
{
    del_timer(&tdev.timer);
}

static void release_workqueue(void)
{
    tdev.read_ready = -1;
    wake_up_interruptible(&tdev.waitq);
    cancel_work_sync(&tdev.work);
    cancel_delayed_work_sync(&tdev.delaywork);
    flush_workqueue(tdev.wq);
    destroy_workqueue(tdev.wq);
    tdev.wq = NULL;
}

static void test_delayed_work_proc(struct work_struct *work)
{
    struct testdev *td = container_of(work, struct testdev, delaywork.work);
    printk(KERN_INFO "%s() l %d, work:%p, ready:%d\n", __func__, __LINE__, work, td->read_ready);
    // queue_delayed_work(td->wq, &td->delaywork, usecs_to_jiffies(100000));
}

static void test_work_proc(struct work_struct *work)
{
    static int n = 0;
    struct testdev *td = container_of(work, struct testdev, work);
    // td->read_ready = 0;
    printk(KERN_INFO "%s() l %d, work:%p, ready:%d\n", __func__, __LINE__, work, td->read_ready);

    wait_event_interruptible(td->waitq, td->read_ready != 0);

    printk(KERN_INFO "%s() l %d\n", __func__, __LINE__);
    if (n < 10) {
        n++;
        msleep(10);
        // queue_work(td->wq, &td->work);
        queue_delayed_work(td->wq, &td->delaywork, usecs_to_jiffies(100000));
    }
}

static void tasklet_func(unsigned long data)
{
    // wake_lock_init(&tdev.wakelock, WAKE_LOCK_SUSPEND, "hehj_wakelock");
    unsigned long j[10] = {0};
    int i = 0;
    j[i++] = jiffies;
    spin_lock(&tdev.lock);
    j[i++] = jiffies;
    udelay(1);
    j[i++] = jiffies;
    udelay(1);
    j[i++] = jiffies;
    udelay(1);
    j[i++] = jiffies;
    udelay(1);
    j[i++] = jiffies;
    spin_unlock(&tdev.lock);
    j[i++] = jiffies;
    udelay(10*1000);
    j[i++] = jiffies;
    udelay(100*100);
    j[i++] = jiffies;

    for (i=i-1; i>=0; i--) {
        printk("j[%d] = %lu\n", i, j[i]);
    }
}


static __s32 __init testdev_module_init(void)
{
    int ret = 0;

    struct timeval tv;
    jiffies_to_timeval(jiffies, &tv);

    tdev.read_ready = 0;

    printk("tv:%ld, %ld\n", tv.tv_sec, tv.tv_usec);
    printk(KERN_INFO "%s() l %d, jiffies:%ld, HZ:%d\n", __func__, __LINE__, jiffies, HZ);

    create_sysfs();

    spin_lock_init(&tdev.lock);
    spin_lock(&tdev.lock);

    // tasklet
    tasklet_init(&tdev.task, tasklet_func, 0L);
    tasklet_schedule(&tdev.task);

    // waitqueue
    init_waitqueue_head(&tdev.waitq);

    init_waitqueue_entry(&tdev.wait, current);

    // timer
    create_timer();

    // workqueue
    tdev.wq = create_singlethread_workqueue("tdev");
    INIT_WORK(&tdev.work, test_work_proc);
    INIT_DELAYED_WORK(&tdev.delaywork, test_delayed_work_proc);

    queue_work(tdev.wq, &tdev.work);
    // queue_delayed_work(tdev.wq, &tdev.delaywork, HZ);

    spin_unlock(&tdev.lock);

    printk(KERN_INFO "%s() l %d\n", __func__, __LINE__);

    return ret;
}

static void __exit testdev_module_exit(void)
{
    printk(KERN_INFO "%s() l %d\n", __func__, __LINE__);
    release_timer();
    release_sysfs();
    release_workqueue();
    tasklet_kill(&tdev.task);
}

module_init(testdev_module_init);
module_exit(testdev_module_exit);

