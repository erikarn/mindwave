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
#include <sys/types.h>
#include <err.h>

#define	SER_PORT		"/dev/cuaU0"

#define MAX_PAYLOAD		169

static const char * eeg_to_str[] = {
	"delta",
	"theta",
	"low-alpha",
	"high-alpha",
	"low-beta",
	"high-beta",
	"low-gamma",
	"mid-gamma",
};

static int
print_payload(const unsigned char *pbuf, int plen)
{
	int i;

	for (i = 0; i < plen; i++) {
		printf("%02x", pbuf[i]);
		if (i % 16 == 15)
			printf("\n");
		else
			printf(" ");
	}
	printf("\n");
	return (plen);
}

static int
parse_payload(const unsigned char *pbuf, int plen)
{
	int i, j, p;
	int len, code;
	int16_t rval;
	uint32_t eeg_power[8];
	int ext_level = 0;

	/*
	 * If we read an 0x80, then the next value is the length of
	 * the section.  Otherwise, it's a single byte value.
	 *
	 * If we read an 0x55, it's an EXCODE byte, and we add one
	 * to the "extended code" value (for use where? I'm not sure.)
	 */
	for (i = 0; i < plen; i++) {
		/*
		 * XXX we should verify that there's enough data available
		 * before we read the next byte in the buffer.
		 */
		code = pbuf[i];

		/*
		 * Count the number of EXCODE bytes we have before 'code'.
		 */
		if (code == 0x55) {
			ext_level++;
			continue;
		}

		if (code & 0x80) {
			/* XXX bounds check */
			i++;
			len = pbuf[i];
		} else {
			len = 1;
		}
//		printf(  "cmd: 0x%02x; len=%d\n", code, len);

		/*
		 * XXX each of these should verify the data is
		 * available!
		 */

		/* 'p' is the data start */
		p = i + 1;

		/* Now we know the payload length, let's parse */
		switch (code) {
		case 2: /* Quality */
			printf("Quality: %d\n", pbuf[p]);
			break;
		case 4: /* Attention */
			printf("Attention: %d\n", pbuf[p]);
			break;
		case 5: /* Meditation */
			printf("Meditation: %d\n", pbuf[p]);
			break;
		case 0x80:
			/* 16-bit 2s complement signed; big endian */
			rval = (pbuf[i+2]) | (pbuf[i+1] << 8);
//			printf("RAW_WAVE: %d\n", rval);
			break;
		case 0x83:
			bzero(&eeg_power, sizeof(eeg_power));
			/* 8 x 24-bit unsigned big endian values */
			/* ASIC_EEG_POWER: the post-processed data */
			for (j = 0; j < 7; j++) {
				eeg_power[j] = (pbuf[p + (j*3)] << 16) | (pbuf[p + (j*3) + 1] << 8) | pbuf[p + (j*3) + 2];
				printf("  %s: (p=%d) %d\n", eeg_to_str[j], p, eeg_power[j]);
			}
			break;
		case 0xd0:
			printf("HEADSET_FOUND_AND_CONNECTED\n");
			break;
		case 0xd1:
			printf("HEADSET_NOT_FOUND\n");
			break;
		case 0xd2:
			printf("HEADSET_DISCONNECTED\n");
			break;
		case 0xd3:
			printf("REQUEST_DENIED\n");
			break;
		case 0xd4:
			printf("DONGLE_STANDBY\n");
			break;
		default:
			printf("Unknown (0x%x)\n", code);
		}
		/* Skip over the correct number of data bytes */
		i += len;
	}
	/* All data consumed */
	return (plen);
}

int
main(int argc, const char *argv[])
{
	int fd;
	struct termios tio;
	unsigned char buf[8];
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
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	cfsetispeed(&tio, B115200);
	cfsetospeed(&tio, B115200);

	if (tcsetattr(fd, TCSAFLUSH, &tio) < 0) {
		err(1, "tcsetattr");
	}

	/* Disconnect command */
	sleep(1);
	buf[0] = 0xc1;
	if (write(fd, buf, 1) != 1)
		printf("SHORT WRITE!\n");
	if (tcdrain(fd) < 0)
		err(1, "tcdrain");

#if 1
	/* Connect to headset 0x430e */
	sleep(1);
	buf[0] = 0xc0;
	buf[1] = 0x87;
	buf[2] = 0x1f;
	if (write(fd, buf, 3) != 3)
		printf("SHORT WRITE!\n");
	if (tcdrain(fd) < 0)
		err(1, "tcdrain");
#else
	/* Search */
	sleep(1);
	buf[0] = 0xc2;
	if (write(fd, buf, 1) != 1)
		printf("SHORT WRITE!\n");
	if (tcdrain(fd) < 0)
		err(1, "tcdrain");
#endif

#if 0
	i = 0;
	while (1) {
		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;
		printf("%02x", buf[0]);
		i++;
		if (i % 32 == 31)
			printf("\n");
		else
			printf(" ");
	}
#endif

	while (1) {
		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;

		(void) gettimeofday(&tv, NULL);

#if 0
		printf("%llu.%06llu: 0x%02x\n",
		    (unsigned long long) tv.tv_sec,
		    (unsigned long long) tv.tv_usec,
		    buf[0]);
#endif

		/*
		 * First two bytes must be SYNC (0xAA)
		 */
		if (buf[0] != 0xaa)
			continue;

		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;

		/*
		 * Next sync byte (0xAA)
		 */
		if (buf[0] != 0xaa)
			continue;

		ret = read(fd, buf, 1);
		if (ret <= 0)
			break;

		plen = buf[0];

		/* Payload must be <= 169 */
		if (plen > 169) {
			printf("Payload too long (%d)\n", plen);
			continue;
		}

//		printf("Payload: %d bytes\n", plen);

		/*
		 * Ok, now we know how much to read, let's read
		 * the next chunk of data from the serial port.
		 */

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
		if (buf[0] != cksum)
			continue;

#if 0
		printf("checksum: %d, generated checksum: %d\n",
		  buf[0],
		  cksum);
#endif

		/* Have a payload! */

		/*
		 * The payload contains multiple data responses.
		 * Some have lengths, some are single byte replies.
		 */
		if (payload[0] != 0x80)
			(void) print_payload(payload, plen);
		(void) parse_payload(payload, plen);
	}

	exit(0);
}
