#include <linux/i2c.h>
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
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
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

#define SCR_DBG(format,args...)   printk("[scr-dbg] "format,##args)
#define SCR_INF(format,args...)   printk("[scr-inf] "format,##args)
#define SCR_ERR(format,args...)   printk("[scr-err] "format,##args)

              
#define LCDC_GCTL_OFF           (0x000)         /*LCD Controller global control registers offset*/
#define LCDC_GINT0_OFF          (0x004)         /*LCD Controller interrupt registers offset*/
#define LCDC_GINT1_OFF          (0x008)         /*LCD Controller interrupt registers offset*/
#define LCDC_FRM0_OFF           (0x010)         /*LCD Controller frm registers offset*/
#define LCDC_FRM1_OFF           (0x014)         /*LCD Controller frm registers offset*/
#define LCDC_FRM2_OFF           (0x02c)         /*LCD Controller frm registers offset*/
#define LCDC_CTL_OFF            (0x040)         /*LCD Controller control registers offset*/
#define LCDC_DCLK_OFF           (0x044)         /*LCD Controller dot clock registers offset*/
#define LCDC_BASIC0_OFF         (0x048)         /*LCD Controller base0 registers offset*/
#define LCDC_BASIC1_OFF         (0x04c)         /*LCD Controller base1 registers offset*/
#define LCDC_BASIC2_OFF         (0x050)         /*LCD Controller base2 registers offset*/
#define LCDC_BASIC3_OFF         (0x054)         /*LCD Controller base3 registers offset*/
#define LCDC_HVIF_OFF           (0x058)         /*LCD Controller hv interface registers offset*/
#define LCDC_CPUIF_OFF          (0x060)         /*LCD Controller cpu interface registers offset*/
#define LCDC_CPUWR_OFF          (0x064)         /*LCD Controller cpu wr registers offset*/
#define LCDC_CPURD_OFF          (0x068)         /*LCD Controller cpu rd registers offset*/
#define LCDC_CPURDNX_OFF        (0x06c)         /*LCD Controller cpu rdnx registers offset*/
#define LCDC_TTL0_OFF           (0x070)         /*LCD Controller TTL0 registers offset*/
#define LCDC_TTL1_OFF           (0x074)         /*LCD Controller TTL1 registers offset*/
#define LCDC_TTL2_OFF           (0x078)         /*LCD Controller TTL2 registers offset*/
#define LCDC_TTL3_OFF           (0x07c)         /*LCD Controller TTL3 registers offset*/
#define LCDC_TTL4_OFF           (0x080)         /*LCD Controller TTL4 registers offset*/
#define LCDC_LVDS_OFF           (0x084)         /*LCD Controller LVDS registers offset*/
#define LCDC_IOCTL0_OFF         (0x088)         /*LCD Controller io control0 registers offset*/
#define LCDC_IOCTL1_OFF         (0x08c)         /*LCD Controller io control1 registers offset*/

#define LCDC_HDTVIF_OFF         (0x090)         /*LCD Controller tv interface  registers offset*/


#define LCDC_HDTV0_OFF          (0x094)         /*LCD Controller HDTV0 registers offset*/
#define LCDC_HDTV1_OFF          (0x098)         /*LCD Controller HDTV1 registers offset*/
#define LCDC_HDTV2_OFF          (0x09c)         /*LCD Controller HDTV2 registers offset*/
#define LCDC_HDTV3_OFF          (0x0a0)         /*LCD Controller HDTV3 registers offset*/
#define LCDC_HDTV4_OFF          (0x0a4)         /*LCD Controller HDTV4 registers offset*/
#define LCDC_HDTV5_OFF          (0x0a8)         /*LCD Controller HDTV5 registers offset*/
#define LCDC_IOCTL2_OFF         (0x0f0)         /*LCD Controller io control2 registers offset*/
#define LCDC_IOCTL3_OFF         (0x0f4)         /*LCD Controller io control3 registers offset*/
#define LCDC_DUBUG_OFF          (0x0fc)         /*LCD Controller debug register*/

#define LCDC_GAMMA_TABLE_OFF    (0x400)

#define LCDC_GET_REG_BASE(sel)  ((sel)==0?(lcdc_reg_base0):(lcdc_reg_base1))

#define LCDC_WUINT32(sel,offset,value)          (*((volatile __u32 *)( LCDC_GET_REG_BASE(sel) + (offset) ))=(value))
#define LCDC_RUINT32(sel,offset)                (*((volatile __u32 *)( LCDC_GET_REG_BASE(sel) + (offset) )))

#define LCDC_SET_BIT(sel,offset,bit)            (*((volatile __u32 *)( LCDC_GET_REG_BASE(sel) + (offset) )) |=(bit))
#define LCDC_CLR_BIT(sel,offset,bit)            (*((volatile __u32 *)( LCDC_GET_REG_BASE(sel) + (offset) )) &=(~(bit)))
#define LCDC_INIT_BIT(sel,offset,c,s)           (*((volatile __u32 *)( LCDC_GET_REG_BASE(sel) + (offset) )) = \
        (((*(volatile __u32 *)( LCDC_GET_REG_BASE(sel) + (offset) )) & (~(c))) | (s)))



static void * lcdc_reg_base0 = NULL, *lcdc_reg_base1=NULL;
static int which = 1; // vga 1 ?
module_param(which, int, 0);

enum {
    DISP_IO_LCDC0 = 0,
    DISP_IO_LCDC1,
};

static struct resource disp_resource[] = {
    [DISP_IO_LCDC0] = {
        .start = 0x01c0c000,
        .end   = 0x01c0cfff,
        .flags = IORESOURCE_MEM,
    },
    [DISP_IO_LCDC1] = {
        .start = 0x01c0d000,
        .end   = 0x01c0dfff,
        .flags = IORESOURCE_MEM,
    },
};

static void dump_lcdc_regs(int sel)
{
    printk("dump_lcdc_regs(%d)>>>>>\n", sel);
    printk("LCDC_HDTV5_OFF: %08x\n", LCDC_RUINT32(sel, LCDC_HDTV5_OFF));
    printk("LCDC_HDTV4_OFF: %08x\n", LCDC_RUINT32(sel, LCDC_HDTV4_OFF));
    printk("LCDC_HDTV3_OFF: %08x\n", LCDC_RUINT32(sel, LCDC_HDTV3_OFF));
    printk("LCDC_HDTV2_OFF: %08x\n", LCDC_RUINT32(sel, LCDC_HDTV2_OFF));
    printk("LCDC_HDTV1_OFF: %08x\n", LCDC_RUINT32(sel, LCDC_HDTV1_OFF));
    printk("LCDC_HDTV0_OFF: %08x\n", LCDC_RUINT32(sel, LCDC_HDTV0_OFF));
    printk("LCDC_IOCTL3_OFF: %08x\n", LCDC_RUINT32(sel, LCDC_IOCTL3_OFF));
    printk("dump_lcdc_regs(%d) done <<<<<\n", sel);
}

static __s32 LCDC_set_int_line(__u32 sel,__u32 tcon_index, __u32 num) 
{
    __u32 tmp = 0; 

    tmp = LCDC_RUINT32(sel, LCDC_GINT0_OFF);

    if(tcon_index==0)
    {    
        LCDC_CLR_BIT(sel,LCDC_GINT0_OFF,1<<29); 
        LCDC_INIT_BIT(sel,LCDC_GINT1_OFF,0x7ff<<16,num<<16);    
    }    
    else 
    {    
        LCDC_CLR_BIT(sel,LCDC_GINT0_OFF,1<<28);     
        LCDC_INIT_BIT(sel,LCDC_GINT1_OFF,0x7ff,num);    
    }    

    LCDC_WUINT32(sel, LCDC_GINT0_OFF, tmp);

    return 0;
}


static int __init screen_module_init(void)
{
    int ret = 0; //-1;
    SCR_INF("hehj screen_module_init() >>>\n");

    struct resource *p;
    p = &disp_resource[DISP_IO_LCDC0];
    lcdc_reg_base0 = ioremap(p->start, p->end - p->start);

    p = &disp_resource[DISP_IO_LCDC1];
    lcdc_reg_base1 = ioremap(p->start, p->end - p->start);

    SCR_DBG("hehj ioremap:%p, %p\n", lcdc_reg_base0, lcdc_reg_base1);

    int sel = 0;
    dump_lcdc_regs(0);
    dump_lcdc_regs(1);
    LCDC_WUINT32(sel, LCDC_HDTV5_OFF, 0x003a005);
    // LCDC_WUINT32(sel, LCDC_HDTV5_OFF, 0x003b005);

    __u32 tmp;
    printk("LCDC_IOCTL3_OFF: %08x\n", (tmp = LCDC_RUINT32(sel, LCDC_IOCTL3_OFF)));
    LCDC_WUINT32(sel, LCDC_IOCTL3_OFF, tmp);

    LCDC_set_int_line(sel, 1, 32);

    dump_lcdc_regs(0);

    return ret;
}

static void __exit screen_module_exit(void)
{
    SCR_INF("hehj screen_module_exit() <<<\n");
    iounmap(lcdc_reg_base0);
    iounmap(lcdc_reg_base1);
}

module_init(screen_module_init);
module_exit(screen_module_exit);

MODULE_AUTHOR("hehj <hehj@routon.com>");
MODULE_DESCRIPTION("SW screen adjust driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:screen");


