/*
author: zhoulong<longer.zhou@gmail.com> 
time: 2013-03-21 
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>

#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/mutex.h>

#include <linux/slab.h>
#include <asm/string.h>

#include <linux/gpio.h>
#include <mach/sys_config.h>

#include "gpio_sw.h"

#define DEBUG_GPIO  0

MODULE_LICENSE("GPL");

static struct regulator_consumer_supply  iovdd_consumers[] = {
   {
      .supply = "idr410_camera_iovdd",
   }
};

static struct regulator_consumer_supply  avdd_consumers[] = {
   {
      .supply = "idr410_camera_avdd",
   },
};

static struct regulator_consumer_supply  dvdd_consumers[] ={
   {
      .supply = "idr410_camera_dvdd",
   },
};

static struct regulator_init_data idr410_regulators_init_data[] = {
   [0] = {  /* camera ioadd */
      .constraints = {
         .name = "iovdd",
         .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
      },
      .num_consumer_supplies =  ARRAY_SIZE(iovdd_consumers),
      .consumer_supplies = iovdd_consumers,
      .supply_regulator = NULL,
   },

   [1] = {  /* camera avdd */
      .constraints = {
         .name = "avdd",
         .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
      },
      .num_consumer_supplies =  ARRAY_SIZE(avdd_consumers),
      .consumer_supplies = avdd_consumers,
      .supply_regulator = NULL,
   },

   [2] = {  /* camera dvdd */
      .constraints = {
         .name = "dvdd",
         .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
      },
      .num_consumer_supplies =  ARRAY_SIZE(dvdd_consumers),
      .consumer_supplies = dvdd_consumers,
      .supply_regulator = NULL,
   },
};


#define REG_NAME_LEN 128
struct idr410_camera_regulator {
    char name[REG_NAME_LEN];
    struct gpio_config *gpio;
    // int active;
    struct regulator_init_data *init_data;
    struct regulator_desc *desc;
    struct regulator_dev *rdev;
};

struct idr410_camera_regulator  idr410_camera_regulators[3];


static int camera_xxx_regulator_is_enabled(struct regulator_dev *rdev)
{
#if 0
    struct idr410_camera_regulator *data = rdev_get_drvdata(rdev);
    int ret;
#endif 
#if DEBUG_GPIO
    printk("====%s \n",__func__);
#endif
    return 0;
}

#if DEBUG_GPIO
static void print_gpio(struct gpio_config *g)
{
    printk("gmpra:(%u %u %u %u %u)", g->gpio, g->mul_sel, g->pull, g->drv_level, g->data);
}
#endif

static int camera_xxx_regulator_enable(struct regulator_dev *rdev)
{
    struct idr410_camera_regulator *data = rdev_get_drvdata(rdev);
    struct gpio_config *gpio = &data->gpio[0];
#if DEBUG_GPIO
    printk("====%s. gpio: %d \n",  __func__, gpio->gpio);
    print_gpio(gpio);
#endif
    return sw_gpio_setall_range(gpio, 1);
}

static int camera_xxx_regulator_disable(struct regulator_dev *rdev)
{
    struct idr410_camera_regulator *data = rdev_get_drvdata(rdev);
    struct gpio_config *gpio = &data->gpio[1];
#if DEBUG_GPIO
    printk("====%s. gpio: %d \n",  __func__, gpio->gpio);
    print_gpio(gpio);
#endif
    return sw_gpio_setall_range(gpio, 1);
}

#if 0
static int camera_xxx_regulator_set_voltage(struct regulator_dev *rdev,
                int min_uV, int max_uV)
{
    return 0;
}

static int camera_xxx_regulator_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
    return 0;
}
#endif

static int camera_xxx_regulator_get_voltage(struct regulator_dev *rdev)
{

    return 0;
}

static int camera_xxx_regulator_suspend_enable(struct regulator_dev *rdev)
{

    return 0;
}

static int camera_xxx_regulator_suspend_disable(struct regulator_dev *rdev)
{

    return 0;
}




static struct regulator_ops camera_xxx_regulator_ops = {
    .list_voltage    = NULL, //XXX_gpio_reg_list_voltage,
    .is_enabled    = camera_xxx_regulator_is_enabled,
    .enable        = camera_xxx_regulator_enable,
    .disable    = camera_xxx_regulator_disable,
    .get_voltage    = camera_xxx_regulator_get_voltage,
    .set_voltage    = NULL,
    .set_suspend_enable    = camera_xxx_regulator_suspend_enable,
    .set_suspend_disable    = camera_xxx_regulator_suspend_disable,
    .set_suspend_voltage    = NULL,
};

struct gpio_config* 
idr410_camera_regulator_parse(char* main_key)
{
    int ret = 0, i;
    script_item_u   val;
    script_item_value_type_e  type;

    char *gpio_keys[] = {"enable", "disable"};
#define keys_num  (sizeof(gpio_keys)/sizeof(char *))

    struct gpio_config *s = kmalloc( sizeof(struct gpio_config) * keys_num, GFP_KERNEL);

    for (i = 0; i < keys_num; i++) {
        type = script_get_item(main_key, gpio_keys[i], &val);
        if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
            printk(" fetech [%s]->%s failed\n", main_key, gpio_keys[i]);
            ret = -1;
            break;
        }
        else {
            memcpy(&s[i].gpio, &val.gpio, sizeof(struct gpio_config));
#if DEBUG_GPIO
            printk(" fetech [%s]->%s finished\n", main_key, gpio_keys[i]);
            print_gpio(&s[i]);
#endif
        }
    }

    if (ret < 0) {
        kfree(s);
        s = NULL;
    }

    return s;
}




#define IDR410_CAMERA_REGULATOR_MAIN_KEY_NAME "idr410_camera_regulators"

int __init
idr410_camera_regulators_init(void)
{
    int i, ret = 0;
    char* sub_keys[] = { "iovdd", "avdd", "dvdd" };
    script_item_u   val;
    script_item_value_type_e  type;

    printk("hehj, keyname = %s\n", IDR410_CAMERA_REGULATOR_MAIN_KEY_NAME);
    /* step 1:   get the infomation of idr410 camera regulators from configuration file */
    for (i = 0; i < sizeof(sub_keys) / sizeof(char *) ; i++) {
        type = script_get_item(IDR410_CAMERA_REGULATOR_MAIN_KEY_NAME, sub_keys[i], &val);
        if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
            ret = -1;
            printk("failed to fetch %s\n", sub_keys[i]);
            goto out;
        } else {
            printk("%s -> %s \n",sub_keys[i], val.str);
            idr410_camera_regulators[i].rdev = 0;
            memcpy(idr410_camera_regulators[i].name, val.str, REG_NAME_LEN);
            idr410_camera_regulators[i].gpio = idr410_camera_regulator_parse(val.str);
            idr410_camera_regulators[i].init_data = &idr410_regulators_init_data[i];

            struct regulator_desc *desc = kmalloc(sizeof(struct regulator_desc), GFP_KERNEL);
            desc->name = kmalloc( strlen(val.str) + 1, GFP_KERNEL);
            memcpy((char *)desc->name, val.str, strlen(val.str) + 1);
            desc->id = i;
            desc->type = REGULATOR_VOLTAGE;
            desc->ops = &camera_xxx_regulator_ops;
            desc->n_voltages = 1;
            desc->owner = THIS_MODULE;
            desc->supply_name = NULL;

            idr410_camera_regulators[i].desc = desc;
        }
    }

#if 1 
    /* step 2: */
    for (i = 0 ; i < ARRAY_SIZE(idr410_camera_regulators); i++) {
#if 0
        struct regulator_init_data *init_data = idr410_camera_regulators[i].init_data;
        struct regulator_desc *regulator_desc = idr410_camera_regulators[i].desc;
        char *supply = NULL;
        printk("init_data:%p\n", init_data);
        printk("init_data->supply_regulator:%p\n", init_data->supply_regulator);
        printk("regulator_desc:%p\n", regulator_desc);
        printk("regulator_desc->supply_name:%p\n", regulator_desc->supply_name);
        /*
        if (init_data && init_data->supply_regulator)
            supply = init_data->supply_regulator;
        else if (regulator_desc->supply_name)
            supply = regulator_desc->supply_name;
            */
        printk("xxxxx supply = '%s' xxxxx\n", supply);
#else
        idr410_camera_regulators[i].rdev = regulator_register(
                idr410_camera_regulators[i].desc, // desc
                NULL,                               // dev
                idr410_camera_regulators[i].init_data, // init data
                &idr410_camera_regulators[i],           // driver_data,
                NULL                    // of_node
                );
        ret = IS_ERR(idr410_camera_regulators[i].rdev);
        if (ret ) {
            printk("camera %s regulator init failed\n", idr410_camera_regulators[i].name);
            goto out;
        } else {
            printk("camera %s regulator init success\n",idr410_camera_regulators[i].name);
            camera_xxx_regulator_disable(idr410_camera_regulators[i].rdev);
        }
#endif
    }
#endif

out:
    return ret;
}


void __exit
idr410_camera_regulators_exit(void)
{
   int i;
   struct regulator_dev *rdev;
   for (i = 0; i < ARRAY_SIZE(idr410_camera_regulators); i++) {
      rdev = idr410_camera_regulators[i].rdev;
      if (rdev)
          regulator_unregister(rdev);
   }
}

module_init(idr410_camera_regulators_init);
module_exit(idr410_camera_regulators_exit);

