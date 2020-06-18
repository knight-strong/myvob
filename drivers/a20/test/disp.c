#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef unsigned char       __u8;
typedef signed short        __s16;
typedef unsigned short      __u16;
typedef signed int          __s32;
typedef unsigned int        __u32;
typedef signed long long    __s64;
typedef unsigned long long  __u64;

#include "drv_display.h"

static void dump_result(const char *name, int ret, int *opdata)
{
    printf("%s : (%d), (%d, %d, %d, %d)\n", name, ret, opdata[0], opdata[1], opdata[2], opdata[3]);
}

#define LCD_OFF     1
#define VGA_OFF     1
#define HDMI_OFF    1

#define LCD_ON      0
#define VGA_ON      1
#define HDMI_ON     0

int main(int argc, char **argv)
{
    int fd = open("/dev/disp", O_RDWR);
    printf("fd:%d\n", fd);
    int32_t opdata[4] = {0};
    int ret;

    ret = ioctl(fd, DISP_CMD_SCN_GET_WIDTH, opdata);
    dump_result("DISP_CMD_SCN_GET_WIDTH", ret, opdata);

    ret = ioctl(fd, DISP_CMD_SCN_GET_HEIGHT, opdata);
    dump_result("DISP_CMD_SCN_GET_HEIGHT", ret, opdata);


#if 0  // unused
    __disp_rectsz_t screen_size;
    screen_size.width = 1280;   screen_size.height = 720;
    //screen_size.width = 64;   screen_size.height = 48;
    opdata[1] = (int32_t)&screen_size;
    ret = ioctl(fd, DISP_CMD_SET_SCREEN_SIZE, opdata);
    dump_result("DISP_CMD_SET_SCREEN_SIZE", ret, opdata);
#endif

#if 0 // unused
    __disp_fb_t para;
    opdata[1] = 100;
    opdata[2] = (int32_t)&para;
    ret = ioctl(fd, DISP_CMD_LAYER_GET_FB, opdata);
    if (ret != 0) {
        perror("ioctl error.");
    }
    dump_result("DISP_CMD_LAYER_GET_FB", ret, opdata);
    printf("fb:%08x, (%u x %u)\n", para.addr[0], para.size.width, para.size.height);

    opdata[0] = 0;
    opdata[1] = 100;
    opdata[2] = (int32_t)&para;
    para.size.width = 1440;
    para.size.height = 900;
    ret = ioctl(fd, DISP_CMD_LAYER_SET_FB, opdata);
    dump_result("DISP_CMD_LAYER_GET_FB", ret, opdata);
#endif

#if 0
    opdata[1] = DISP_TV_MOD_720P_60HZ; //  DISP_TV_MOD_1080P_60HZ;//  DISP_TV_MOD_720P_60HZ;
    ret = ioctl(fd, DISP_CMD_HDMI_SET_MODE, opdata);
    dump_result("DISP_CMD_HDMI_SET_MODE", ret, opdata);
#endif
    if (VGA_ON) {
        opdata[1] = DISP_VGA_H1440_V900;
        //opdata[1] = DISP_VGA_H1280_V720;
        // opdata[1] = DISP_VGA_H1024_V768;
        //opdata[1] = DISP_VGA_H1360_V768;
        ret = ioctl(fd, DISP_CMD_VGA_SET_MODE, opdata);
        dump_result("DISP_CMD_VGA_SET_MODE", ret, opdata);
    }

    // turn off / on
    if (HDMI_OFF) {
        ret = ioctl(fd, DISP_CMD_HDMI_OFF, opdata);
        dump_result("DISP_CMD_HDMI_OFF", ret, opdata);
    }

    if (VGA_OFF) {
        ret = ioctl(fd, DISP_CMD_VGA_OFF, opdata);
        dump_result("DISP_CMD_VGA_OFF", ret, opdata);

    }
    
    if (LCD_OFF) {
        ret = ioctl(fd, DISP_CMD_LCD_OFF, opdata);
        dump_result("DISP_CMD_LCD_OFF", ret, opdata);
    }

    printf("closed...\n");
    sleep(3);
    printf("reopen...\n");

#if 1
    if (HDMI_ON) {
        ret = ioctl(fd, DISP_CMD_HDMI_ON, opdata);
        dump_result("DISP_CMD_HDMI_ON", ret, opdata);
    }

    if (VGA_ON) {
        ret = ioctl(fd, DISP_CMD_VGA_ON, opdata);
        dump_result("DISP_CMD_VGA_ON", ret, opdata);
    }

    // ret = ioctl(fd, DISP_OUTPUT_TYPE_TV, opdata);
    //dump_result("DISP_CMD_TV_ON", ret, opdata);

    if (LCD_ON) {
        ret = ioctl(fd, DISP_OUTPUT_TYPE_LCD, opdata);
        dump_result("DISP_OUTPUT_TYPE_LCD", ret, opdata);
    }
#endif

#if 0
    opdata[1] = DISP_VGA_H1440_V900;
    //opdata[1] = DISP_VGA_H1280_V720;
    //opdata[1] = DISP_VGA_H1024_V768;
    ret = ioctl(fd, DISP_CMD_VGA_SET_MODE, opdata);
    dump_result("DISP_CMD_VGA_SET_MODE", ret, opdata);

    ret = ioctl(fd, DISP_CMD_VGA_ON, opdata);
    dump_result("DISP_CMD_VGA_ON", ret, opdata);
#endif
    close(fd);
    return 0;
}

