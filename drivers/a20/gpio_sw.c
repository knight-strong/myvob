#include <linux/input.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif
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
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/irqs.h>
#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <linux/init-input.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <mach/sys_config.h>
#include "gpio_sw.h"

#define FAKE_VER_FOR_8188ETV 1  // for debug: should be 0 for release

#define SXM_DBG_LEVEL   3

#if (SXM_DBG_LEVEL == 1)
        #define SXM_DBG(format,args...)   printk("[sxm-dbg] "format,##args)
        #define SXM_INF(format,args...)   printk("[sxm-inf] "format,##args)
        #define SXM_ERR(format,args...)   printk("[sxm-err] "format,##args)
#elif (SXM_DBG_LEVEL == 2)
        #define SXM_DBG(format,args...)
        #define SXM_INF(format,args...)   printk("[sxm-inf] "format,##args)
        #define SXM_ERR(format,args...)   printk("[sxm-err] "format,##args)
#elif (SXM_DBG_LEVEL == 3)
        #define SXM_DBG(format,args...)
        #define SXM_INF(format,args...)
        #define SXM_ERR(format,args...)   printk("[sxm-err] "format,##args)
#endif

#define SXM_DBG_FUN_LINE_TODO           printk("%s, line %d, todo############\n", __func__, __LINE__)
#define SXM_DBG_FUN_LINE                printk("%s, line %d\n", __func__, __LINE__)
#define SXM_ERR_FUN_LINE                printk("%s err, line %d\n", __func__, __LINE__)


struct gpio_def {
    int gpio;
    char * name;
};
static struct gpio_def gpio_items[] = {
    { GPIOA(0),     "sunxi_gpio" },
    { GPIOA(8),     "PA8"  },
    { GPIOA(12),    "PA12" },
    { GPIOB(8),     "PB8"  },
    { GPIOB(9),     "PB9"  },
    { GPIOB(10),    "PB10" },
    { GPIOG(0),     "PG0"  },
    { GPIOG(1),     "PG1"  },
    { GPIOG(2),     "PG2"  },
    { GPIOG(3),     "PG3"  },
    { GPIOG(5),     "PG5"  },
    { GPIOG(8),     "PG8"  },
    { GPIOG(10),    "PG10" },
    { GPIOG(11),    "PG11" },
    { GPIOB(12),    "PB12" },
    { GPIOC(0),     "PC0"  },
    { GPIOC(1),     "PC1"  },
    { GPIOC(2),     "PC2"  },
    { GPIOC(3),     "PC3"  },
    { GPIOC(21),    "PC21" },
    { GPIOC(22),    "PC22" },
    { GPIOH(19),    "PH19" },
    { GPIOH(20),    "PH20" },
};
#define gpio_count (sizeof(gpio_items)/sizeof(struct gpio_def))

static struct cdev      *g_cdev = NULL;
static dev_t            g_devid = -1;
static struct class     *g_class = NULL;
static struct device            *g_dev[gpio_count] = { NULL };
static struct platform_device   *board_device = NULL;

static char* pm_usb_wifi_port = "PG5";  //  PG5/PG3 for media smaller/larger board
static char* pm_usb_3g_port = "PG3";    //  PG5/PG3 for media smaller/larger board
module_param(pm_usb_wifi_port, charp, 0444);
module_param(pm_usb_3g_port, charp, 0444);
MODULE_PARM_DESC(pm_usb_wifi_port, "which io port used by pm for wifi");
MODULE_PARM_DESC(pm_usb_3g_port, "which io port used by pm for 3g");

static int gpio_cfg_out(int gpio, int data)
{
    struct gpio_config gio = {
        .gpio = gpio,
        .mul_sel = GPIO_CFG_OUTPUT,
        .pull = 1,
        .drv_level = 1,
        .data = data
    };
    int r = gpio_request(gio.gpio, NULL);
    if (unlikely(r)){
         SXM_ERR(KERN_ERR "ERROR: Gpio_request failed\n");
    }
    else {
        r = sw_gpio_setall_range(&gio, 1);
    }
    gpio_free(gio.gpio);
    return r;
}

static int gpio_cfg_in(int gpio)
{
    struct gpio_config gio = {
        .gpio = gpio,
        .mul_sel = GPIO_CFG_INPUT,
        .pull = 0,
        .drv_level = 1,
        .data = 0
    };
    int r = gpio_request(gio.gpio, NULL);
    if (unlikely(r)){
         SXM_ERR(KERN_ERR "ERROR: Gpio_request failed\n");
    }
    else {
        r = sw_gpio_setall_range(&gio, 1);
    }
    gpio_free(gio.gpio);
    return r;
}

static int gpio_cfg_func(int gpio, int data)
{
    struct gpio_config gio = {
        .gpio = gpio,
        .mul_sel = data,
        .pull = 1,
        .drv_level = 1,
        .data = 0
    };
    int r = gpio_request(gio.gpio, NULL);
    if (unlikely(r)){
         SXM_ERR(KERN_ERR "ERROR: Gpio_request failed\n");
    }
    else {
        r = sw_gpio_setall_range(&gio, 1);
    }
    gpio_free(gio.gpio);
    return r;
}

static int gpio_cfg_request(int gpio, int data)
{
    struct gpio_config gio = {
        .gpio = gpio,
        .mul_sel = data,
        .pull = 1,
        .drv_level = 1,
        .data = 0
    };
    int r = gpio_request(gio.gpio, NULL);
    if (unlikely(r)){
         SXM_ERR(KERN_ERR "ERROR: Gpio_request failed\n");
         gpio_free(gio.gpio);
    }
    else {
        gpio_free(gio.gpio);
    }

    return r;
}

static int init_board_gpio(void)
{
    script_item_u *gpio_hd = NULL;
    int i, gpio_cnt, ret;
    gpio_cnt = script_get_pio_list("gpio_para", &gpio_hd);
    if (unlikely(!gpio_cnt && !gpio_hd)) {
        SXM_ERR("ERROR: Request gpio resources is failed!\n");
    } 
    else {
        for (i = 0; i < gpio_cnt; i++){
            ret = gpio_request(gpio_hd[i].gpio.gpio, NULL);
            if (unlikely(ret)){
                SXM_ERR("ERROR: Gpio_request: %d is failed. ret:%d\n", i, ret); 
                // goto gpio_err;
            }   
        }   

        SXM_DBG("hehj gpio_sw, init gpio_cnt:%d\n", gpio_cnt);
        ret = sw_gpio_setall_range(&gpio_hd[0].gpio, gpio_cnt);
        if (unlikely(ret)){
            SXM_ERR("ERROR: gpio set all range is error!\n");
            goto gpio_err;
        } 
    }
gpio_err:
    for (i = 0; i < gpio_cnt; i++) {
        gpio_free(gpio_hd[i].gpio.gpio);
    }

#if 0
    /* card reader */
    gpio_cfg_out(GPIOG(1), 1);
    gpio_cfg_out(GPIOA(12), 1);


    /* finger */
    gpio_cfg_out(GPIOA(8), 1);
#endif
    return 0;
}

static int sunmm_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}

static int sunmm_release(struct inode *inode, struct file *file)
{
    return 0;
}

long sunmm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0, which, data;
    switch (cmd) {
        case GPIOC_SETOUTPUT:
            get_user(which, (int *)arg);
            get_user(data, (int *)arg + 1);
            SXM_DBG("which: %d, out data: %d\n", which, data);
            ret = gpio_cfg_out(which, data);
            break;
        case GPIOC_SETINPUT:
            get_user(which, (int *)arg);
            ret = gpio_cfg_in(which);
            break;
        case GPIOC_SETFUNCTION:
            get_user(which, (int *)arg);
            get_user(data, (int *)arg + 1);
            SXM_DBG("which: %d, function: %d\n", which, data);
            ret = gpio_cfg_func(which, data);
            break;
        case GPIOC_REQUEST:
            get_user(which, (int *)arg);
            get_user(data, (int *)arg + 1);
            SXM_DBG("request test >>> which: %d, function: %d\n", which, data);
            ret = gpio_cfg_request(which, data);
            SXM_DBG("request test done <<<\n");
            break;
        default:
            ret = -ENOIOCTLCMD;
    }
    return ret;
}

static struct file_operations sunxi_gpio_fops = {
    .owner          = THIS_MODULE,
    .open           = sunmm_open,
    .release        = sunmm_release,
    .unlocked_ioctl = sunmm_ioctl,
};

static ssize_t sysfs_show_data(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct gpio_def * gpio = (struct gpio_def *)dev_get_drvdata(dev);
    gpio_cfg_in(gpio->gpio);

    int value = __gpio_get_value(gpio->gpio);
#if FAKE_VER_FOR_8188ETV
    if (gpio->gpio == GPIOC(0))
        value = 1;
#endif
    return sprintf(buf, "%d\n", value);
}

static ssize_t sysfs_set_data(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct gpio_def * gpio = (struct gpio_def *)dev_get_drvdata(dev);
    int value = simple_strtol(buf, NULL, 10);
    SXM_DBG("set %s to %s\n", gpio->name, value ? "H" : "L");
    gpio_cfg_out(gpio->gpio, value ? 1 : 0);
    return strnlen(buf, count);
}

static struct device_attribute sysfs_attrs[] = {
    __ATTR(data, S_IRUGO | S_IWUSR, sysfs_show_data, sysfs_set_data),
    { },
};

static ssize_t show_pmu_enabled(struct device *dev, struct device_attribute *attr, char *buf)
{
    static int enabled = -1;
    if (enabled == -1) {
        script_item_u   val;
        if ((script_get_item("pmu_para", "pmu_suspend_enable", &val) == SCIRPT_ITEM_VALUE_TYPE_INT) 
                && (val.val == 0)) {
            enabled = 0;
        }
        else {
            enabled = 1;
        }
    }
    else; // do nothing

    // enabled = 0; // test
    return sprintf(buf, "%d\n", enabled);
}

static ssize_t show_pm_wifi_port(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s", pm_usb_wifi_port);
}

static ssize_t show_pm_3g_port(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s", pm_usb_3g_port);
}

static ssize_t show_product_name(struct device *dev, struct device_attribute *attr, char *buf)
{
    script_item_u   val;
    if (script_get_item("usb_feature", "product_name", &val) == SCIRPT_ITEM_VALUE_TYPE_STR) {
        return sprintf(buf, "%s\n", val.str);
    }
    
    return sprintf(buf, "%s\n", "UNKOWN");
}

static ssize_t show_gps_enabled(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d", 0);
}

static DEVICE_ATTR(pmu_enabled, S_IRUGO, show_pmu_enabled, NULL);
static DEVICE_ATTR(pm_wifi_port, S_IRUGO, show_pm_wifi_port, NULL);
static DEVICE_ATTR(pm_3g_port, S_IRUGO, show_pm_3g_port, NULL);
static DEVICE_ATTR(product_name, S_IRUGO, show_product_name, NULL);
static DEVICE_ATTR(gps_enabled, S_IRUGO, show_gps_enabled, NULL);

static struct attribute *board_sysfs_entries[] = {
         &dev_attr_pmu_enabled.attr,
         &dev_attr_pm_wifi_port.attr,
         &dev_attr_pm_3g_port.attr,
         &dev_attr_product_name.attr,
         &dev_attr_gps_enabled.attr,
         NULL
};

static struct attribute_group system_attribute_group = {
    .name = NULL,
    .attrs = board_sysfs_entries,
};

static int __init gpio_module_init(void)
{
    int i = 0;
    int ret = 0;
    SXM_INF("hehj gpio_sw_init (111) in, count:%d>>>\n", gpio_count);

    /* 初始化gpio口的状态 */
    init_board_gpio();

    ret = alloc_chrdev_region(&g_devid, 0, gpio_count, "sunxi_gpio");
    if(ret) {
        SXM_ERR_FUN_LINE;
        return ret;
    }

    g_cdev = cdev_alloc();
    if(NULL == g_cdev) {
        SXM_ERR_FUN_LINE;
        goto out1;
    } 
    cdev_init(g_cdev, &sunxi_gpio_fops);
    g_cdev->owner = THIS_MODULE;
    ret = cdev_add(g_cdev, g_devid, gpio_count); 
    if(ret) {
        SXM_ERR_FUN_LINE;
        goto out2;
    } 

    board_device = platform_device_register_simple("board", -1, NULL, 0);
    ret = sysfs_create_group(&board_device->dev.kobj, &system_attribute_group);
    if (ret) {
        SXM_ERR("sysfs_create_group() for board error.\n");
    }

    /* class create and device register */
    g_class = class_create(THIS_MODULE, "gpio_sw_hehj");
    if(IS_ERR(g_class)) {
        SXM_ERR_FUN_LINE;
        goto out3;
    }
    g_class->dev_attrs = sysfs_attrs;

    for (i=0; i<gpio_count; i++) {
        g_dev[i] = device_create(g_class, NULL, MKDEV(MAJOR(g_devid), i), &gpio_items[i], gpio_items[i].name);
        if(IS_ERR(g_dev[i])) {
            SXM_ERR_FUN_LINE;
            goto out4;
        }
    }

    SXM_DBG("%s success, line %d\n", __func__, __LINE__);
    return ret;

out4:
    class_destroy(g_class);
    g_class = NULL;
out3:
    cdev_del(g_cdev);
out2:
    g_cdev = NULL;
out1:
    unregister_chrdev_region(g_devid, 1);
    g_devid = -1;

    return ret;
}

static void __exit gpio_module_exit(void)
{
    SXM_DBG("hehj gpio_sw_exit <<<\n");

    platform_device_unregister(board_device);

    int i;
    for (i=0; i<gpio_count; i++) {
        device_destroy(g_class, MKDEV(MAJOR(g_devid), i));
        g_dev[i] = NULL;
    }

    class_destroy(g_class);
    g_class = NULL;

    /* release char dev */
    cdev_del(g_cdev);
    g_cdev = NULL;

    unregister_chrdev_region(g_devid, 1);
    g_devid = -1;
}

module_init(gpio_module_init);
module_exit(gpio_module_exit);

MODULE_AUTHOR("hehaojun <hehj@routon.com>");
MODULE_DESCRIPTION("SW GPIO driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:gpio_sw");

