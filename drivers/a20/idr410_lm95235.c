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

#define LM95235_MAINKEY "idr410_LM95235"
#define LM95235_INT_KEY "T_CRIT_int"

#define PH03_EXT_INT_NUM  3

#define PIO_INT_STAT_OFFSET          (0x214)
#define PIO_INT_CTRL_OFFSET          (0x210)


#define LM95235_ADDR 0x4c
#define ADAPTER_NUM 2

#define LM95235_READ_LOCAL_TEMP    1
#define LM95235_READ_REMOTE_TEMP   2
#define LM95235_READ_MANUFACTUREID 3
#define LM95235_READ_REMOTE_T_CRIT_LIMIT 4
#define LM95235_SET_REMOTE_T_CRIT_LIMIT  5
#define LM95235_READ_CONFIGURATION_REGISTER_2 6
#define LM95235_SET_CONFIGURATION_REGISTER_2 7

static struct gpio_config gpio_int;

static struct i2c_board_info lm95235 = {
    .addr = LM95235_ADDR,
    .platform_data = NULL,
};

struct idr410_temperature_dev {
    struct class *sys_class;
    struct device *sys_device;
    dev_t devno;

    int localTemp;
    int remoteTemp;
    int remote_T_Crit_Limit;
    unsigned char configReg2;

    struct i2c_client *i2cClinet;
    struct work_struct work;

    u32 cd_hdle;
};


struct idr410_temperature_dev *lm95235_dev = NULL;


static int lm95235_operate(struct i2c_client *client, int cmd, unsigned int value);

static void 
lm95235_init_default(struct idr410_temperature_dev *dev)
{
    if (dev == NULL) {
        return ;
    }

    lm95235_operate(dev->i2cClinet, LM95235_SET_CONFIGURATION_REGISTER_2, 0x31);
    lm95235_operate(dev->i2cClinet, LM95235_SET_REMOTE_T_CRIT_LIMIT, 75);
}

static void temp_exceed(struct work_struct *ws)
{
    /* do something when  the temperature of idr410 exceeds the limit. zhoulong */
    printk("[WARNING:] received the interrupt from lm95235 that indicates "
           "the temperature of idr410"
           "exceeds the Limit you setted.\n");
    kernel_power_off();  //zhoulong
}

static irqreturn_t idr410_temp_isr(void *dev_data)
{
    struct idr410_temperature_dev *priv = (struct idr410_temperature_dev *)dev_data;
    schedule_work(&priv->work);
    return 0;
}

struct idr410_temperature_dev* alloc_lm95235_dev(void)
{
    struct idr410_temperature_dev *dev = (struct idr410_temperature_dev*)kmalloc(sizeof(*dev), GFP_KERNEL);

    struct i2c_adapter  *adapter = i2c_get_adapter(ADAPTER_NUM);
    if (adapter == NULL) {
        printk("[ERROR] failed to i2c_get_adapter\n");
        goto err;
    }

    struct i2c_client* client = i2c_new_device(adapter, &lm95235);
    if (client == NULL) {
        printk("[ERROR] failed to i2c_new_device\n");
        goto err;
    }
    dev->i2cClinet = client;

    dev->cd_hdle = sw_gpio_irq_request(gpio_int.gpio, TRIG_EDGE_NEGATIVE,
            &idr410_temp_isr, dev);
    if (!dev->cd_hdle) {
        printk("Failed to get gpio irq for lm95235\n");
        goto err;
    }

    INIT_WORK(&dev->work, temp_exceed);
   
   // lm95235_init_default(dev);

    return dev;

err:
    if (dev) {
        kfree(dev);
    }

    return NULL;
}

static int lm95235_operate(struct i2c_client *client, int cmd, unsigned int value)
{
    int ret = 0;

    switch (cmd) {
    case LM95235_READ_MANUFACTUREID:
    {
        s32 val;
        val = i2c_smbus_read_byte_data(client, 0xfe);
        printk(" val = %d\n", val);
        break;
    }
    case LM95235_READ_LOCAL_TEMP:
    {
        /*
        Local Temperature MSB : (Read Only Address 00h)
           Bit      D7     D6      D5      D4      D3      D2      D1      D0
           Value    SIGN   64      32      16      8       4       2       1
         Temperature Data: Lsb = 1 degree Celsius
         
         Local Temperature LSB : (Read Only Address 30h)
           Bit      D7     D6      D5      D4      D3      D2      D1      D0
           Value    0.5    0.25    0.125   0       0       0       0       0
         Temperature Data Lsb = 0.125 degree Celsius
        */
        s32 msb, lsb;
        msb = i2c_smbus_read_byte_data(client, 0x00);
        lsb = i2c_smbus_read_byte_data(client, 0x30);
        if (msb < 0 || lsb < 0) {
            printk("ERROR: failed to read Local Temp from LM95235\n");
            ret = -1;
        } else {
            //s32 localTemp = (msb & 0x00ff) | ((lsb & 0x0007) << 8);
            s32 localTemp = (msb & 0x007f); 
            if (msb & 0x0080) 
                localTemp *= -1; 
            printk("localTemp = %d\n", localTemp);

            lm95235_dev->localTemp = localTemp;
        }
        break;
    }
    case LM95235_READ_REMOTE_TEMP:
    {
        /*
        Unsigned Remote Temperature MSB : (Read Only Address 31h) 
            Bit      D7     D6      D5      D4      D3      D2      D1      D0
           Value    128   64      32      16      8       4       2       1
          Temperature Data: Lsb = 1 degree Celsius
        Unsigned Remote Temperature LSB, Filter On :(Read Only Address 32h) 
            Bit      D7     D6      D5      D4      D3      D2      D1      D0
           Value    0.5     0.25    0.125   0.0625  0.03125 0       0       0
        Unsinged Remote Temperature LSB, Filter off: (Read Only Address 32h)
           Bit      D7     D6      D5      D4      D3      D2      D1      D0
           Value    0.5    0.25    0.125   0       0       0       0       0
         Temperature Data: Lsb = 0.125  degree Celsius when filter off  and,
                                0.03125 egree Celsius when filter on
         
        */
        s32 msb, lsb;
        msb = i2c_smbus_read_byte_data(client, 0x31);
        lsb = i2c_smbus_read_byte_data(client, 0x32);
        if (msb < 0 || lsb < 0) {
            printk("ERROR: failed to read Local Temp from LM95235\n");
            ret = -1;
        } else {
           // s32 remoteTemp = (msb & 0x00ff) | ((lsb & 0x0007) << 8);
            s32 remoteTemp = msb & 0x00ff;
            printk("remoteTemp = %d\n", remoteTemp);

            lm95235_dev->remoteTemp = remoteTemp;
        }
        break;
    }
    case LM95235_READ_REMOTE_T_CRIT_LIMIT:
    {
        s32 limit;
        limit = i2c_smbus_read_byte_data(client, 0x19);
        if (limit < 0) {
            printk("ERROR: failed to read remote T_Crit limit from LM95235\n");
            ret = -1;
        } else {
            printk("remote_T_Crit_Limit = %d\n", limit);
            lm95235_dev->remote_T_Crit_Limit = limit;
        }
        break;
    }
    case LM95235_SET_REMOTE_T_CRIT_LIMIT:
    {
        unsigned char arg = (unsigned char)(value & 0xff);
        s32 rc = i2c_smbus_write_byte_data(client, 0x19, arg);
        if (!rc) {
            lm95235_dev->remote_T_Crit_Limit = arg;
        }
        break;
    }
    case LM95235_READ_CONFIGURATION_REGISTER_2:
    {
        s32 reg = i2c_smbus_read_byte_data(client, 0xbf);
        if (reg < 0) {
            printk("ERROR: failed to read configuration register 2 from LM95235");
            ret = -1;
        } else {
            printk("configuration register : %02x\n", reg & 0xff);
            lm95235_dev->configReg2 = (unsigned char)(reg & 0xff);
        }
        break;
    }
    case LM95235_SET_CONFIGURATION_REGISTER_2:
    {
        unsigned char arg = (unsigned char)(value  & 0xff);
        s32 re = i2c_smbus_write_byte_data(client, 0xbf, arg);
        if (!re) {
            lm95235_dev->configReg2 = arg;
        }
        break;
    }
    default:
        printk("sorry ,unsurpported cmd\n");
        break;

    }
    return ret;
}


static ssize_t
sysfs_show_localTemp(struct device *dev, struct device_attribute *attr, char *buf)
{
        lm95235_operate(lm95235_dev->i2cClinet, LM95235_READ_LOCAL_TEMP, 0);
        int localTemp = lm95235_dev->localTemp;

        return sprintf(buf, "%d\n", localTemp);
}
static ssize_t
sysfs_show_remoteTemp(struct device *dev, struct device_attribute *attr, char *buf)
{
        lm95235_operate(lm95235_dev->i2cClinet, LM95235_READ_REMOTE_TEMP, 0);
        int remoteTemp = lm95235_dev->remoteTemp;
        return sprintf(buf, "%d\n", remoteTemp);
}
/*

*/
static ssize_t
sysfs_show_remote_T_Crit_Limit(struct device *dev, struct device_attribute *attr, char *buf)
{
        lm95235_operate(lm95235_dev->i2cClinet, LM95235_READ_REMOTE_T_CRIT_LIMIT, 0);
        int remote_T_Crit_Limit = lm95235_dev->remote_T_Crit_Limit;
        return sprintf(buf, "%d\n", remote_T_Crit_Limit);
}

/*

*/
static ssize_t
sysfs_set_remote_T_Crit_Limit(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
        printk("get string = %s\n", buf);
        long value = simple_strtol(buf, NULL, 10);
        if (value > 0) {
            lm95235_operate(lm95235_dev->i2cClinet, LM95235_SET_REMOTE_T_CRIT_LIMIT, value);
        }

        return value > 0 ? n : -1;
}
/*

*/
static struct device_attribute sysfs_attrs[] = {
        __ATTR(localTemp, S_IRUGO, sysfs_show_localTemp, NULL),
        __ATTR(remoteTemp, S_IRUGO, sysfs_show_remoteTemp, NULL),
        __ATTR(remote_T_Crit_Limit, S_IRUGO | S_IWUSR, sysfs_show_remote_T_Crit_Limit, sysfs_set_remote_T_Crit_Limit),
        //__ATTR(max_user_freq, S_IRUGO | S_IWUSR, sysfs_show_max_user_freq, sysfs_set_max_user_freq),
        { },
};

static int sysfs_suspend(struct device *dev, pm_message_t mesg)
{
        printk("\n");
        return 0;
}

static int sysfs_resume(struct device *dev)
{
        printk("\n");
        return 0;
}

static int __init lm95235_init(void)
{
        int ret;
        printk("\n");

        script_item_u   val;
        script_item_value_type_e  type;

        //config gpio to int mode
        printk("%s: config gpio to int mode. \n", __func__);

        type = script_get_item(LM95235_MAINKEY, LM95235_INT_KEY, &val);
        if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
            printk(" fetch [%s.%s] failed\n", LM95235_MAINKEY, LM95235_INT_KEY);
            return -1;
        }

        memcpy(&gpio_int, &val.gpio, sizeof(struct gpio_config));


        struct idr410_temperature_dev *dev = alloc_lm95235_dev();
        if (dev == NULL) {
            printk("ERROR: alloc_lm95235_dev\n");
            return -1;
        }

        dev->sys_class = class_create(THIS_MODULE, "idr410_temperature");
        if (IS_ERR(dev->sys_class)) {
                printk(KERN_ERR "%s: couldn't create class\n", __FILE__);
                return PTR_ERR(dev->sys_class);
        }

        dev->sys_class->suspend = sysfs_suspend;
        dev->sys_class->resume = sysfs_resume;
        dev->sys_class->dev_attrs = sysfs_attrs;

        ret = alloc_chrdev_region(&dev->devno, 0, 1, "idr410_temp");
        if (ret < 0) {
            printk("ERROR: alloc_chrdev_region");
        }

        dev->sys_device = device_create(dev->sys_class, NULL, dev->devno, NULL, "idr410_temp");
        if (IS_ERR(dev->sys_device)) {
            printk("ERROR: device_create failed\n");
            return PTR_ERR(dev->sys_device);
        }

        lm95235_dev = dev;
        lm95235_init_default(dev);

        return 0;
}

static void __exit lm95235_exit(void)
{
        struct idr410_temperature_dev *dev = lm95235_dev;
        
        if (dev != NULL) {
            sw_gpio_irq_free(dev->cd_hdle);
            unregister_chrdev_region(dev->devno, 1);
            device_destroy(dev->sys_class, dev->devno);
            class_destroy(dev->sys_class);
            kfree(dev);
        }

        gpio_free(gpio_int.gpio);

        lm95235_dev = NULL;
}

module_init(lm95235_init);
module_exit(lm95235_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

