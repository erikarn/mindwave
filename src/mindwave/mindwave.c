/*
 * Tinker with the mindwave USB widget thing.
 *
 * Unfortunately! This thing doesn't work right now - I get valid
 * packets but it never pairs.  I guess that I have to speak some
 * undocumented commands to actually scan and pair with a headset
 * before it starts spitting out data.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <err.h>

#define	SER_PORT		"/dev/cuaU0"

#define MAX_PAYLOAD		169

int
main(int argc, const char *argv[])
{
	int fd;
	struct termios tio;
	unsigned char buf[1];
	unsigned char payload[MAX_PAYLOAD];
	int ret;
	int plen;
	int i;
	unsigned char cksum;
	struct timeval tv;

	fd = open(SER_PORT, O_RDWR);
	if (fd < 0) {
		err(1, "open");
	}

	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	cfsetispeed(&tio, B115200);
	cfsetospeed(&tio, B115200);

	if (tcsetattr(fd, TCSANOW, &tio) < 0) {
		err(1, "tcsetattr");
	}
	if (tcsetattr(fd, TCSAFLUSH, &tio) < 0) {
		err(1, "tcsetattr");
	}

	sleep(3);

#if 1
	/* XX no idea what this does? */
	buf[0] = 194;
	if (write(fd, buf, 1) != 1)
		printf("SHORT WRITE!\n");
#endif

	while (1) {
		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;

		(void) gettimeofday(&tv, NULL);

#if 1
		printf("%llu.%06llu: 0x%02x\n",
		    (unsigned long long) tv.tv_sec,
		    (unsigned long long) tv.tv_usec,
		    buf[0]);
#else
		if (buf[0] != 170)
			continue;

		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;

		if (buf[0] != 170)
			continue;

		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;
		plen = buf[0];
		if (plen > 169) {
			printf("Payload > 169 (%d)\n", plen);
			continue;
		}
		printf("Payload: %d bytes\n", plen);

		cksum = 0;
		for (i = 0; i < plen; i++) {
			ret = read(fd, &payload[i], 1);
			if (ret <= 0)
				break;
			cksum += payload[i];
		}
		if (i != plen) {
			printf("Short read?\n");
			continue;
		}

		/* Take ones compliment of generated checksum */
		cksum = 255 - cksum;

		/* Checksum */
		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;
		printf("checksum: %d, generated checksum: %d\n",
		  buf[0],
		  cksum);

		/* Have a payload! */
		for (i = 0; i < plen; i++) {
			switch (payload[i]) {
				case 2: /* Quality */
					i++;
					printf("Quality: %d\n", payload[i]);
					break;

				case 4: /* Attention */
					i++;
					printf("Attention: %d\n", payload[i]);
					break;

				case 5: /* Meditation */
					i++;
					printf("Meditation: %d\n", payload[i]);
					break;
				default:
					printf("Unknown (0x%x)\n", payload[i]);
			}
		}
#endif
	}

	exit(0);
}
