#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <wiringPi.h>

#define BAUDRATE B1000000
#define SHUTTER_PIN 0

volatile int trigCam = 0;

void isr()
{
	trigCam = 1;
}

int main()
{
	int fd;
	struct termios newtio;
	char fbuf[1024];
	char buf[256];

	wiringPiSetup();
	pinMode(SHUTTER_PIN, INPUT);
	pullUpDnControl(SHUTTER_PIN, PUD_UP);
	wiringPiISR(SHUTTER_PIN, INT_EDGE_FALLING, &isr);

	fd = open("/dev/ttyS0", O_RDWR|O_NOCTTY);
	if(fd<0) {
		fprintf(stderr, "failed to open port: %s.\r\n", strerror(errno));
		printf("Make sure you are executing in sudo.\r\n");
		return 1;
	}
	usleep(250000);

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_cflag = BAUDRATE|CS8|CLOCAL|CREAD;
	newtio.c_iflag = ICRNL;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 1;
	
//	speed_t baudRate = B1000000;
//	cfsetispeed(&newtio, baudRate);
//	cfsetospeed(&newtio, baudRate);

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	while(1) {
		if(trigCam) {
			system("raspistill -w 640 -h 480 -t 10 -o image.jpg");
			usleep(10000);
			int img = open("./image.jpg", O_RDONLY);
			if(img > 0) {
				ssize_t rd_size;
				do {
					rd_size = read(img, fbuf, 1024);
					write(fd, fbuf, rd_size);
				} while(rd_size > 0);
			}
			else {
				printf("failed to open file\r\n");
			}
			trigCam = 0;
		}
	}
	return 0;
}
