#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define USERSPACE
#include "../gpio_sw.h"

const char * dev = "/dev/sunxi_gpio";

struct io_argv {
    int which;
    int data;
};

static int init_lvds_lcd(int fd, int lvds)
{
    printf("set lvds or lcd: %d\n", lvds);
    int func = lvds ? 3 : 2;
    struct io_argv arg;
    for (int i=0; i<=19; i++) { 
        arg.which = GPIOD(i);
        ioctl(fd, GPIOC_SETFUNCTION, &arg);
    }
    return 0;
}

static int test_gpio_request(int fd)
{
#if 1
    int r;
    struct io_argv arg;
    arg.which = GPIOC(22);
    arg.data = 1;
    r = ioctl(fd, GPIOC_REQUEST, &arg);
    printf("ioctl(%d), r=%d\n", arg.which, r);

    arg.which = GPIOC(21);
    r = ioctl(fd, GPIOC_REQUEST, &arg);
    printf("ioctl(%d), r=%d\n", arg.which, r);
#else
    int r;
    struct io_argv arg;
    for (int i=0; i<=17; i++) { 
        arg.which = GPIOA(i);
        arg.data = 2;
        r = ioctl(fd, GPIOC_REQUEST, &arg);
        printf("ioctl(%d), r=%d\n", i, r);
    }
#endif
    return 0;
}

static int set_pwm_func(int fd)
{
    printf("set to pwm\n");
    struct io_argv arg;
    arg.which = GPIOB(2);
    arg.data = 2;
    ioctl(fd, GPIOC_SETFUNCTION, &arg);
    return 0;
}

int main(int argc, char** argv)
{
    int dir_out;
    struct io_argv arg;
    if (argc < 2) {
        printf("usage: %s <gpio> [data]\n", argv[0]);
        exit(-1);
    }
    if (argc > 2) {
        dir_out = 1;
        arg.data = atoi(argv[2]);
    }
    else {
        dir_out = 0;
    }

    int fd = open(dev, O_RDWR);
    if (fd != -1) {
    }
    else {
        fprintf(stderr, "open %s failed. %d\n", dev, errno);
        return -1;
    }

    char *w = argv[1];
    int io = atoi(argv[1]+1);
    switch (w[0]) {
        case 'a':
        case 'A':
            arg.which = GPIOA(io);
            break;
        case 'b':
        case 'B':
            arg.which = GPIOB(io);
            break;
        case 'c':
        case 'C':
            arg.which = GPIOC(io);
            break;
        case 'd':
        case 'D':
            arg.which = GPIOD(io);
            break;
        case 'e':
        case 'E':
            arg.which = GPIOE(io);
            break;
        case 'f':
        case 'F':
            arg.which = GPIOF(io);
            break;
        case 'g':
        case 'G':
            arg.which = GPIOG(io);
            break;
        case 'h':
        case 'H':
            arg.which = GPIOH(io);
            break;
        case 'i':
        case 'I':
            arg.which = GPIOI(io);
            break;
        case '0':
            init_lvds_lcd(fd, 0);
            goto exit_done;
        case '1':
            init_lvds_lcd(fd, 1);
            goto exit_done;
        case '2':
            test_gpio_request(fd);
            goto exit_done;
        case '3':
            set_pwm_func(fd);
            goto exit_done;
        default:
            fprintf(stderr, "invalid port %s\n", argv[1]);
            break;
    }
    if (dir_out) {
        ioctl(fd, GPIOC_SETOUTPUT, &arg);
    }
    else {
        ioctl(fd, GPIOC_SETINPUT, &arg);
    }

exit_done:
    close(fd);
    return 0;
}

