/* longer.zhou@gmail.com */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mach/sys_config.h>
#include <linux/slab.h>

#if 1
#define RF_MSG(...)     do {printk("[idr410_gps_rfkill]: "__VA_ARGS__);} while(0)
#else
#define RF_MSG(...)
#endif


#define IDR410_GPS_MAIN_KEY  "idr410_gps"


static DEFINE_SPINLOCK(gps_power_lock);
static const char gps_name[] = "idr410_gps";
static struct rfkill *sw_rfkill;
static struct gpio_config *s;

static bool gps_disable_status ;

/* * @set_block: turn the transmitter on (blocked == false) or off
 *	(blocked == true) -- ignore and return 0 when hard blocked.
 */
static int rfkill_set_power(void *data, bool blocked)
{
    struct gpio_config *g = (struct gpio_config *) data;
    struct gpio_config *gpio ;
    RF_MSG("rfkill set power %d\n", !blocked);
    
    spin_lock(&gps_power_lock);
   
    if (!blocked) { 
        gpio = &g[0];
    } else {
        gpio = &g[1];
    }

    sw_gpio_setall_range(gpio, 1);

    gps_disable_status = blocked;
    spin_unlock(&gps_power_lock);
    msleep(100);
    return 0;
}

/* export a function to*/
int gps_rfkill_set_power(bool blocked)
{
    return rfkill_set_power(s,blocked);
}
EXPORT_SYMBOL(gps_rfkill_set_power);

bool get_gps_disable_status(void)
{
    return gps_disable_status;
}

EXPORT_SYMBOL(get_gps_disable_status);
/**/

static struct rfkill_ops gps_rfkill_ops = {
    .set_block = rfkill_set_power,
};

static int gps_rfkill_probe(struct platform_device *pdev)
{
    int ret = 0, i;
    char *sub_keys[] = {"enable", "disable"};
    #define keys_num  (sizeof(sub_keys)/sizeof(char *))

    script_item_u   val;
    script_item_value_type_e  type;


    s = kmalloc(sizeof(struct gpio_config) * keys_num, GFP_KERNEL);

    for (i = 0; i < keys_num; i++) {
       type = script_get_item(IDR410_GPS_MAIN_KEY, sub_keys[i], &val);
        if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
            ret = -1;
            printk(" fetech [%s]->%s failed\n", IDR410_GPS_MAIN_KEY, sub_keys[i]);
            goto out;
        }
        else {
            memcpy(&s[i], &val.gpio, sizeof(struct gpio_config));
            printk(" fetech [%s]->%s finished\n", IDR410_GPS_MAIN_KEY, sub_keys[i]);
        }
    }

out:
    if (ret < 0) {
       kfree(s);
       s = NULL;
       return -EINVAL;
    }
    
    sw_rfkill = rfkill_alloc(gps_name, &pdev->dev, 
                        RFKILL_TYPE_GPS, &gps_rfkill_ops, s);
    if (unlikely(!sw_rfkill))
        return -ENOMEM;

    rfkill_init_sw_state(sw_rfkill, true);  //2013-09-04

    ret = rfkill_register(sw_rfkill);
    if (unlikely(ret)) {
        rfkill_destroy(sw_rfkill);

        if (s) {
           kfree(s);
           s = NULL;
        }
    }
    return ret;
}

static int gps_rfkill_remove(struct platform_device *pdev)
{
    if (likely(sw_rfkill)) {
        rfkill_unregister(sw_rfkill);
        rfkill_destroy(sw_rfkill);
    }

    if (s) {
       kfree(s);
       s = NULL;
    }
    return 0;
}

static int gps_rfkill_suspend(struct platform_device *pdev, pm_message_t state)
{
    printk("%s L_%d\n", __FUNCTION__, __LINE__);
    return 0;
}

static int gps_rfkill_resume(struct platform_device *pdev)
{
    printk("%s L_%d\n", __FUNCTION__, __LINE__);
    return 0;
}

static struct platform_driver gps_rfkill_driver = {
    .probe = gps_rfkill_probe,
    .remove = gps_rfkill_remove,
    .suspend = gps_rfkill_suspend,
    .resume = gps_rfkill_resume,
    .driver = { 
        .name = "idr410-gps-rfkill",
        .owner = THIS_MODULE,
    },
};

static struct platform_device gps_rfkill_dev = {
    .name = "idr410-gps-rfkill",
};

static int __init gps_rfkill_init(void)
{
    platform_device_register(&gps_rfkill_dev);
    return platform_driver_register(&gps_rfkill_driver);
}

static void __exit gps_rfkill_exit(void)
{
    platform_device_unregister(&gps_rfkill_dev);
    platform_driver_unregister(&gps_rfkill_driver);
}

module_init(gps_rfkill_init);
module_exit(gps_rfkill_exit);

MODULE_DESCRIPTION("idr410-gps-rfkill driver");
MODULE_AUTHOR("zhoulong<longer.zhou@gmail.com>");
MODULE_LICENSE("GPL");



