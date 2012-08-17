/*
 
 sendrawpdu (c) 2012 pod2g
 
 Command line tool, usage: sendrawpdu <PDU data in hex>
 
 - Code based on iphone-elite's sendmodem ( http://code.google.com/p/iphone-elite/wiki/sendmodem )
 - Just few modifications for SMS sending and iPhone 4 compatibility.
 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#define BUFSIZE (65536+100)
unsigned char readbuf[BUFSIZE];

static struct termios term;
static struct termios gOriginalTTYAttrs;

void sendCmd(int fd, void *buf, size_t size);
void sendStrCmd(int fd, char *buf);
int readResp(int fd);
int initConn(int speed);
void closeConn(int fd);
void sendAt(int fd);
void at(int fd);

void sendCmd(int fd, void *buf, size_t size) {
    if(write(fd, buf, size) == -1) {
        fprintf(stderr, "sendCmd error. %s\n", strerror(errno));
        exit(1);
    }
}

void sendStrCmd(int fd, char *buf) {
    sendCmd(fd, buf, strlen(buf));
}

int readResp(int fd) {
    int len = 0;
    struct timeval timeout;
    int nfds = fd + 1;
    fd_set readfds;
    int select_ret;
    
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    
    // Wait a second
    timeout.tv_sec = 1;
    timeout.tv_usec = 500000;
    
    while ((select_ret = select(nfds, &readfds, NULL, NULL, &timeout)) > 0) {
        len += read(fd, readbuf + len, BUFSIZE - len);
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
    }
    if (len > 0) {
    }
    readbuf[len] = 0;
    fprintf(stderr,"%s",readbuf);
    return len;
}

int initConn(int speed) {
    int fd = open("/dev/dlci.spi-baseband.extra_0", O_RDWR | O_NOCTTY);
    
    if(fd == -1) {
        fprintf(stderr, "%i(%s)\n", errno, strerror(errno));
        exit(1);
    }
    
    ioctl(fd, TIOCEXCL);
    fcntl(fd, F_SETFL, 0);
    
    tcgetattr(fd, &term);
    gOriginalTTYAttrs = term;
    
    cfmakeraw(&term);
    cfsetspeed(&term, speed);
    term.c_cflag = CS8 | CLOCAL | CREAD;
    term.c_iflag = 0;
    term.c_oflag = 0;
    term.c_lflag = 0;
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSANOW, &term);
    
    return fd;
}

void closeConn(int fd) {
    tcdrain(fd);
    tcsetattr(fd, TCSANOW, &gOriginalTTYAttrs);
    close(fd);
}

void sendAt(int fd) {
    char cmd[5];
    sprintf(cmd,"AT\r");
    sendCmd(fd, cmd, strlen(cmd));
}

void at(int fd) {
    sendAt(fd);
    for (;;) {
        if(readResp(fd) != 0) {
            if(strstr((const char *)readbuf,"OK") != NULL) {
                break;
            }
        }
        sendAt(fd);
    }
}

int main(int argc, char **argv) {
    int fd;
    char cmd[1024];
    if(argc < 2) {
        fprintf(stderr,"usage: %s <pdu data>\n",argv[0]);
        exit(1);
    }
    fd = initConn(115200);
    at(fd);
    sendStrCmd(fd, "AT+CMGF=0\r");
    readResp(fd);
    sprintf(cmd, "AT+CMGS=%ld\r", strlen(argv[1])/2 - 1);
    sendStrCmd(fd, cmd);
    readResp(fd);
    sprintf(cmd,"%s\032",argv[1]);
    sendStrCmd(fd, cmd);
    readResp(fd);
    closeConn(fd);
    return 0;
}
