/* i2c test
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <asm/atomic.h>

#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/init-input.h>

#define MMA_printk(...)  do { printk("[i2c_rtc]: "__VA_ARGS__); } while(0)
#define MMA_pr_err(...)  do { pr_err("[i2c_rtc]: "__VA_ARGS__); } while(0)
#define MMA_pr_info(...)  do { pr_info("[i2c_rtc]: "__VA_ARGS__); } while(0)
#define MMA_pr_notice(...)  do { pr_notice("[i2c_rtc]: "__VA_ARGS__); } while(0)

/*
 * Defines
 */
#define assert(expr)\
    do {if (!(expr)) {\
        MMA_printk(KERN_ERR "Assertion failed! %s, %d, %s, \"%s\"\n",\
            __FILE__, __LINE__, __func__, #expr);\
    }} while(0)

#define assert_return(expr, retval)\
    do {if (!(expr)) {\
        MMA_printk(KERN_ERR "Assertion failed! %s, %d, %s, \"%s\"(returned)\n",\
            __FILE__, __LINE__, __func__, #expr);\
        return retval;\
    }} while(0)

#define i2c_rtc_DRV_NAME        "i2c_rtc"
#define SENSOR_NAME             i2c_rtc_DRV_NAME
#define i2c_rtc_XOUTL           0x00
#define i2c_rtc_XOUTH           0x01
#define i2c_rtc_YOUTL           0x02
#define i2c_rtc_YOUTH           0x03
#define i2c_rtc_ZOUTL           0x04
#define i2c_rtc_ZOUTH           0x05
#define i2c_rtc_XOUT8           0x06
#define i2c_rtc_YOUT8           0x07
#define i2c_rtc_ZOUT8           0x08
#define i2c_rtc_STATUS          0x09
#define i2c_rtc_DETSRC          0x0A
#define i2c_rtc_TOUT            0x0B
#define i2c_rtc_OC_RESERVED     0x0C
#define i2c_rtc_I2CAD           0x0D
#define i2c_rtc_USRINF          0x0E
#define i2c_rtc_WHOAMI          0x0F
#define i2c_rtc_XOFFL           0x10
#define i2c_rtc_XOFFH           0x11
#define i2c_rtc_YOFFL           0x12
#define i2c_rtc_YOFFH           0x13
#define i2c_rtc_ZOFFL           0x14
#define i2c_rtc_ZOFFH           0x15
#define i2c_rtc_MCTL            0x16
#define i2c_rtc_INTRST          0x17
#define i2c_rtc_CTL1            0x18
#define i2c_rtc_CTL2            0x19
#define i2c_rtc_LDTH            0x1A
#define i2c_rtc_PDTH            0x1B
#define i2c_rtc_PW              0x1C
#define i2c_rtc_LT              0x1D
#define i2c_rtc_TW              0x1E
#define i2c_rtc_1F_RESERVED     0x1F

#define POLL_INTERVAL_MAX    500
#define POLL_INTERVAL        100
#define INPUT_FUZZ    2
#define INPUT_FLAT    2

/* LD or PD, includes xDX, xDY, xDZ */
#define MK_i2c_rtc_DETSRC(LD, PD, INT)\
    (LD<<5 | PD<<2 | INT)

#define MK_i2c_rtc_MCTL(DRPD, SPI3W, STON, GLVL, MOD)\
    (DRPD<<6 | SPI3W<<5 | STON<<4 | GLVL<<2 | MOD)

#define MK_i2c_rtc_CTL1(DFBW, THOPT, ZDA, YDA, XDA, INTRG, INTPIN)\
    (DFBW<<7 | THOPT<<6 | ZDA<<5 | YDA<<4 | XDA<<3 | INTRG<<1 | INTPIN)

#define MK_i2c_rtc_CTL2(DRVO, PDPL, LDPL)\
    (DRVO<<2 | PDPL<<1 | LDPL)

#define MK_i2c_rtc_DRIFT_OFFL(OFFSET)\
    ((OFFSET) & 0xFF)
#define MK_i2c_rtc_DRIFT_OFFH(OFFSET)\
    ((((OFFSET) & 0x7FF) >> 8) & 0x7)

#define i2c_rtc_STATUS_DRDY_MASK 1
#define i2c_rtc_STATUS_DOVR_MASK 2
#define i2c_rtc_STATUS_PERR_MASK 4

#define GET_i2c_rtc_STATUS_DRDY(STATUS)\
    ((STATUS) & i2c_rtc_STATUS_DRDY_MASK)
#define GET_i2c_rtc_STATUS_DOVR(STATUS)\
    ((STATUS) & i2c_rtc_STATUS_DOVR_MASK)
#define GET_i2c_rtc_STATUS_PERR(STATUS)\
    ((STATUS) & i2c_rtc_STATUS_PERR_MASK)

#define MK_i2c_rtc_10BIT_OUT(OUTH, OUTL)\
    ((((OUTH) & 0x3) << 8) | ((OUTL) & 0xFF))

#define MODE_CHANGE_DELAY_MS 50
#define OPERATE_DELAY_US 1000

#define i2c_rtc_LSB_PER_G 64
#define i2c_rtc_CALIBRATION_DEVIATION 2

#define i2c_rtc_ENABLE 1
#define i2c_rtc_DISABLE 0

static struct i2c_client *i2c_rtc_i2c_client;

/* for calibration */
static atomic_t calibrating = ATOMIC_INIT(1);
// static s16 xyz_cal[3] = {0, 0, 0};

struct i2c_rtc_data_s {
    atomic_t enable;
    struct i2c_client       *client;
    struct input_polled_dev *pollDev; 
    struct mutex interval_mutex; 
    struct mutex init_mutex;
} i2c_rtc_data;


static struct input_polled_dev *i2c_rtc_idev;

/* Addresses to scan */
static union{
    unsigned short dirty_addr_buf[2];
    const unsigned short normal_i2c[2];
}u_i2c_addr = {{0x00},};
static __u32 twi_id = 0;

static u32 debug_mask = 0xff;
#define dprintk(level_mask, fmt, arg...)        if (unlikely(debug_mask & level_mask)) \
            printk(KERN_DEBUG fmt , ## arg)


static int gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
    
    if(twi_id == adapter->nr){
        MMA_pr_info("%s: Detected chip %s at adapter %d, address 0x%02x\n",
             __func__, SENSOR_NAME, i2c_adapter_id(adapter), client->addr);

        strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
        return 0;
    }else{
        return -ENODEV;
    }
}

static int __devinit i2c_rtc_probe(struct i2c_client *client,
                   const struct i2c_device_id *id)
{
    dprintk(DEBUG_INIT, "i2c_rtc probe end\n");
    return 0;
}

static int __devexit i2c_rtc_remove(struct i2c_client *client)
{
    int result;
    i2c_set_clientdata(client, NULL);
    return result;
}

static const struct i2c_device_id i2c_rtc_id[] = {
    { i2c_rtc_DRV_NAME, 1 },
    { }
};
MODULE_DEVICE_TABLE(i2c, i2c_rtc_id);

static struct i2c_driver i2c_rtc_driver = {
    .class = I2C_CLASS_HWMON,
    .driver = {
        .name    = i2c_rtc_DRV_NAME,
        .owner    = THIS_MODULE,
    },
    //.suspend = i2c_rtc_suspend,
    //.resume    = i2c_rtc_resume,
    .probe    = i2c_rtc_probe,
    .remove    = __devexit_p(i2c_rtc_remove),
    .id_table = i2c_rtc_id,
    .address_list    = u_i2c_addr.normal_i2c,
};

static int __init i2c_rtc_init(void)
{
    int ret = -1;
    MMA_printk("======%s=========. \n", __func__);

    u_i2c_addr.dirty_addr_buf[0] = 0x51;
    u_i2c_addr.dirty_addr_buf[1] = I2C_CLIENT_END;
    twi_id = 1;

    MMA_printk("%s: after fetch_sysconfig_para:  normal_i2c: 0x%hx. normal_i2c[1]: 0x%hx \n", \
    __func__, u_i2c_addr.normal_i2c[0], u_i2c_addr.normal_i2c[1]);

    i2c_rtc_driver.detect = gsensor_detect;

    ret = i2c_add_driver(&i2c_rtc_driver);
    if (ret < 0) {
        MMA_printk(KERN_INFO "add i2c_rtc i2c driver failed\n");
        return -ENODEV;
    }
    MMA_printk(KERN_INFO "add i2c_rtc i2c driver\n");

    return ret;
}

static void __exit i2c_rtc_exit(void)
{
    MMA_printk(KERN_INFO "remove i2c_rtc i2c driver.\n");
    i2c_del_driver(&i2c_rtc_driver);
}

MODULE_AUTHOR("Chen Gang <gang.chen@freescale.com>");
MODULE_DESCRIPTION("i2c_rtc 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(i2c_rtc_init);
module_exit(i2c_rtc_exit);

