#ifndef _LINUX_GPIO_SW_H
#define _LINUX_GPIO_SW_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define GPIO_SW_IOCTL_BASE          'P'
#define GPIOC_SETOUTPUT             _IOW(GPIO_SW_IOCTL_BASE, 1, int)
#define GPIOC_SETINPUT              _IOW(GPIO_SW_IOCTL_BASE, 2, int)
#define GPIOC_SETFUNCTION           _IOW(GPIO_SW_IOCTL_BASE, 3, int)
#define GPIOC_REQUEST               _IOW(GPIO_SW_IOCTL_BASE, 4, int)


#ifdef USERSPACE
typedef unsigned int        __u32;
typedef unsigned int          u32;

/* port number for each pio */
#define PA_NR           18
#define PB_NR           24
#define PC_NR           25
#define PD_NR           28
#define PE_NR           12
#define PF_NR           6
#define PG_NR           12
#define PH_NR           28
#define PI_NR           22

#ifdef CONFIG_AW_AXP20
#define AXP_NR          5
#endif

/*
 * base index for each pio
 */
#define SUN7I_GPIO_SPACE    5 /* for debugging purposes so that failed if request extra gpio_nr */
#define AW_GPIO_NEXT(gpio)  gpio##_NR_BASE + gpio##_NR + SUN7I_GPIO_SPACE + 1
enum sun7i_gpio_number {
    PA_NR_BASE = 0,                 /* 0    */
    PB_NR_BASE = AW_GPIO_NEXT(PA),  /* 24   */
    PC_NR_BASE = AW_GPIO_NEXT(PB),  /* 54   */
    PD_NR_BASE = AW_GPIO_NEXT(PC),  /* 85   */
    PE_NR_BASE = AW_GPIO_NEXT(PD),  /* 119  */
    PF_NR_BASE = AW_GPIO_NEXT(PE),  /* 137  */
    PG_NR_BASE = AW_GPIO_NEXT(PF),  /* 149  */
    PH_NR_BASE = AW_GPIO_NEXT(PG),  /* 167  */
    PI_NR_BASE = AW_GPIO_NEXT(PH),  /* 201  */
#ifdef CONFIG_AW_AXP20
    AXP_NR_BASE     = AW_GPIO_NEXT(PI),     /* 229 */
    GPIO_INDEX_END  = AW_GPIO_NEXT(AXP),    /* 240 */
#else
    GPIO_INDEX_END  = AW_GPIO_NEXT(PI),     /* 229 */
#endif
};

/* pio index definition */
#define GPIOA(n)        (PA_NR_BASE + (n))
#define GPIOB(n)        (PB_NR_BASE + (n))
#define GPIOC(n)        (PC_NR_BASE + (n))
#define GPIOD(n)        (PD_NR_BASE + (n))
#define GPIOE(n)        (PE_NR_BASE + (n))
#define GPIOF(n)        (PF_NR_BASE + (n))
#define GPIOG(n)        (PG_NR_BASE + (n))
#define GPIOH(n)        (PH_NR_BASE + (n))
#define GPIOI(n)        (PI_NR_BASE + (n))
#ifdef CONFIG_AW_AXP20
#define GPIO_AXP(n)     (AXP_NR_BASE + (n))
#endif

/* pio default macro */
#define GPIO_PULL_DEFAULT   ((u32)-1         )
#define GPIO_DRVLVL_DEFAULT ((u32)-1         )
#define GPIO_DATA_DEFAULT   ((u32)-1         )

/* pio end, invalid macro */
#define GPIO_INDEX_INVALID  (0xFFFFFFFF      )
#define GPIO_CFG_INVALID    (0xFFFFFFFF      )
#define GPIO_PULL_INVALID   (0xFFFFFFFF      )
#define GPIO_DRVLVL_INVALID (0xFFFFFFFF      )
#define IRQ_NUM_INVALID     (0xFFFFFFFF      )

#define AXP_PORT_VAL        (0x0000FFFF      )

/* config value for external int */
#define GPIO_CFG_EINT       (0b110  )   /* config value to eint for ph/pi */

#define GPIO_CFG_INPUT      (0)
#define GPIO_CFG_OUTPUT     (1)

/* port number for gpiolib */
#ifdef ARCH_NR_GPIOS
#undef ARCH_NR_GPIOS
#endif
#define ARCH_NR_GPIOS       (GPIO_INDEX_END)

/* gpio config info */
struct gpio_config {
    u32 gpio;       /* gpio global index, must be unique */
    u32 mul_sel;    /* multi sel val: 0 - input, 1 - output... */
    u32 pull;       /* pull val: 0 - pull up/down disable, 1 - pull up... */
    u32 drv_level;  /* driver level val: 0 - level 0, 1 - level 1... */
    u32 data;       /* data val: 0 - low, 1 - high, only vaild when mul_sel is input/output */
};

#endif /* USERPACE */

#endif

