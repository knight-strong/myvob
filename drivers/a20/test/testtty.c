#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <termios.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <pthread.h>

#include <linux/types.h>
#include <linux/ioctl.h>


static int
epoll_register( int  epoll_fd, int  fd ) 
{
    struct epoll_event  ev;  
    int                 ret, flags;

    /* important: make the fd non-blocking */
    flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    ev.events  = EPOLLIN;
    ev.data.fd = fd;
    do { 
        ret = epoll_ctl( epoll_fd, EPOLL_CTL_ADD, fd, &ev );
    } while (ret < 0 && errno == EINTR);
    return ret; 
}

static void * gps_state_thread(void *fd)
{
    int  epoll_fd   = epoll_create(2);
    int gps_fd = (int)fd;
    epoll_register( epoll_fd, gps_fd );
    epoll_register( epoll_fd, 1 );
    for (;;) {
        struct epoll_event   events[2];
        int                  ne, nevents;

        nevents = epoll_wait( epoll_fd, events, 2, -1 );
        if (nevents < 0) {
            if (errno != EINTR)
                printf("epoll_wait() unexpected error: %s", strerror(errno));
            else
                printf("epoll_wait error: %s", strerror(errno));
            continue;
        }
        printf("gps thread received %d events\n", nevents);

        for (ne = 0; ne < nevents; ne++) {
            if ((events[ne].events & (EPOLLERR|EPOLLHUP)) != 0) {
                printf("EPOLLERR or EPOLLHUP after epoll_wait() !?");
                exit(-1);
            }
            if ((events[ne].events & EPOLLIN) != 0) {
                printf("evt fd %d in\n", events[ne].data.fd);
            }
            else {
                printf("unknown evt\n");
            }
        }
    }

    return NULL;
}

static int uart_speed(int s)
{
    switch (s) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        case 230400:
            return B230400;
        case 460800:
            return B460800;
        case 500000:
            return B500000;
        case 576000:
            return B576000;
        case 921600:
            return B921600;
        case 1000000:
            return B1000000;
        case 1152000:
            return B1152000;
        case 1500000:
            return B1500000;
        case 2000000:
            return B2000000;
#ifdef B2500000
        case 2500000:
            return B2500000;
#endif
#ifdef B3000000
        case 3000000:
            return B3000000;
#endif
#ifdef B3500000
        case 3500000:
            return B3500000;
#endif
#ifdef B4000000
        case 4000000:
            return B4000000;
#endif
        default:
            return B57600;
    }
}


int set_speed(int fd, struct termios *ti, int speed)
{
    if (cfsetospeed(ti, uart_speed(speed)) < 0) 
        return -errno;

    if (cfsetispeed(ti, uart_speed(speed)) < 0) 
        return -errno;

    if (tcsetattr(fd, TCSANOW, ti) < 0) 
        return -errno;

    return 0;
}


int main(int argc, char** argv)
{
    unsigned char buf[256];
    char file[256];
    sprintf(file, "/dev/ttyS%s", argv[1]);
    printf("read %s\n", file);
    int fd = open(file, O_RDWR | O_NOCTTY);
    int i;
    // int fd = open("/dev/ttyS3", O_RDWR);
    if (fd == -1) {
        printf("open error. %s\n", strerror(errno));
        return -1;
    }
    if ( isatty(fd) ) {
        tcflush(fd, TCIOFLUSH);
        struct termios  ti;
        tcgetattr( fd, &ti );
         cfmakeraw(&ti);
        ti.c_cflag |= CLOCAL;
        ti.c_cflag &= ~CRTSCTS;
        //ti.c_oflag &= (~ONLCR); /* Stop \n -> \r\n translation on output */
        //ti.c_iflag &= (~(ICRNL | INLCR)); /* Stop \r -> \n & \n -> \r translation on input */
        //ti.c_iflag |= (IGNCR | IXOFF);  /* Ignore \r & XON/XOFF on input */
        // cfsetispeed(&ti, B4800); 
        if (tcsetattr(fd, TCSANOW, &ti) < 0) {
            perror("Can't set port attr");
            return -1;
        }

        if (set_speed(fd, &ti, 9600) < 0) {
            perror("Can't set port speed");
            return -1;
        }

        tcflush(fd, TCIOFLUSH);

        //ti.c_cflag |= PARENB;
        //ti.c_cflag &= ~(PARODD);

        ti.c_cflag &= ~PARENB;
        ti.c_cflag &= ~INPCK;
        
        if (tcsetattr(fd, TCSANOW, &ti) < 0) { 
            perror("Can't set port settings");
            return -1;
        }    
        if (0) {
            // send sync
            unsigned char init_data[8] = { 0xc0, 0, 0x2f, 0, 0xd0, 0x1, 0x7e, 0xc0 };
            int l = write(fd, init_data, 8);
            printf("write init_data, r=%d\n", l);
        }
    }
    else {
        printf("not a tty ?\n");
        close(fd);
        return -2;
    }

#if 0
    printf("%s, %d\n", __FUNCTION__, __LINE__);
    pthread_t p;
    if ( pthread_create( &p, NULL, gps_state_thread, (void *)fd ) != 0 ) {
        printf("%s, %d\n", __FUNCTION__, __LINE__);
        printf("could not create gps thread: %s", strerror(errno));
        exit(2);
    }
    while (1) sleep(1);
#else 
#if 0 
    buf[0]='a';
    while (0) {
        int l = write(fd, buf, 1);
        printf("write: %d\n", l);
    }
#else
    int n = 0;
    
    while (1) {
        {
            
            unsigned char testdata[256]={0xFF, 0xFE, 0xFD, 0xFC, 0xFB};/*{0xC0, 0x01, 0x03, 0xFB, 0xC0};*/
            //sprintf(testdata, "[%d]hello,%s", n++, file);
            int l = write(fd, testdata, 5);
            printf("\n[%d]write, r=%d / 5,", n, l);
            for (i=0; i<5; i++)  printf("%02x ", testdata[i]);
            printf("\n");
        }
        //sleep(1);
        int l = read(fd, buf, sizeof(buf));
        if (l > 0) {
            buf[l] = 0;
            n++;
            printf("[%d]read len: %d,   ", n, l);
            
            for (i=0; i<l; i++)  printf("%02x ", (unsigned char)buf[i]);
            printf("\n");
        }
        else {
            if (l < 0) {
                printf("read error:%s\n", strerror(errno));
                break;
            }
            // printf("read: [null]\n");
        }
    }
#endif
#endif
    close(fd);
    return 0;
}

