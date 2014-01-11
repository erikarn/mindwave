#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>
#include <sys/endian.h>

#include "mindwave_hw.h"
#include "mindwave.h"

#define	PKT_DEBUG		0

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

struct mindwave_hdl *
mindwave_new(void)
{
	struct mindwave_hdl *mw;

	mw = calloc(1, sizeof(*mw));
	if (mw == NULL) {
		warn("%s: calloc", __func__);
		return (NULL);
	}

	mw->ms_fd = -1;
	mw->read_buf.len = 1024;
	mw->read_buf.buf = malloc(mw->read_buf.len);
	if (mw->read_buf.buf == NULL) {
		warn("%s: malloc", __func__);
		free(mw);
		return (NULL);
	}

	return (mw);
}

int
mindwave_set_serial(struct mindwave_hdl *mw, const char *sport)
{

	/* XXX if connected, error */

	if (mw->ms_sport != NULL) {
		free(mw->ms_sport);
	}

	mw->ms_sport = strdup(sport);
	return (0);
}

int
mindwave_open(struct mindwave_hdl *mw)
{
	struct termios tio;
	int fd;

	/* No serial port? */
	if (mw->ms_sport == NULL) {
		/* XXX error code! */
		return (-1);
	}

	/* Already open? */
	if (mw->ms_fd != -1) {
		/* XXX error code! */
		return (-1);
	}

	fd = open(mw->ms_sport, O_RDWR);
	if (fd < 0) {
		warn("%s: open", __func__);
		/* XXX error code! */
		return (-1);
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
		warn("%s: tcsetattr", __func__);
		close(fd);
		/* XXX error code! */
		return (-1);
	}

	mw->ms_fd = fd;
	mw->ms_state = MS_IDLE;

	return (0);
}

/*
 * XXX for now just send the command, don't bother checking state
 * and such.
 */
int
mindwave_connect_headset(struct mindwave_hdl *mw, uint16_t headset_id)
{
	char buf[3];
	int r;

	buf[0] = 0xc0;
	buf[1] = (headset_id >> 8) & 0xff;
	buf[2] = (headset_id) & 0xff;

	if (write(mw->ms_fd, buf, 3) != 3) {
		return (-1);
	}
	return (0);
}

int
mindwave_send_disconnect(struct mindwave_hdl *mw)
{
	char buf[2];
	int r;

	buf[0] = 0xc1;

	if (write(mw->ms_fd, buf, 1) != 1) {
		return (-1);
	}
	return (0);
}

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

static uint8_t
mindwave_calc_checksum(const uint8_t *buf, int len)
{
	uint8_t cksum;
	int i;

	cksum = 0;
	for (i = 0; i < len; i++) {
		cksum += buf[i];
	}

	/* Take ones compliment of generated checksum */
	cksum = 255 - cksum;

	return (cksum);
}

static int
mindwave_parse_payload(const unsigned char *pbuf, int plen)
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

/*
 * Parse a single buffer worth of data out
 * of the incoming buffer.
 *
 * Return -1 on error, 0 if no data was consumed,
 * > 0 if data was consumed and should be taken
 * from the read buffer.
 *
 * XXX I'm not convinced this is correct yet!
 */
static int
mindwave_parse_buffer(struct mindwave_hdl *mw)
{
	const unsigned char *buf;
	int i = 0;
	int len;
	uint32_t plen;
	uint8_t cksum;

	buf = (unsigned char *) mw->read_buf.buf;
	len = mw->read_buf.offset;

#if PKT_DEBUG
	printf("Called: len=%d bytes\n", len);
#endif

	/*
	 * Read data until we hit a sync byte.
	 *
	 * We'll discard the data in the buffer up until
	 * we find two sync bytes.  We don't consume
	 * the two sync bytes because we may end up
	 * hitting some partial packet scenario and
	 * we definitely want to buffer those.
	 */

	/*
	 * Do we have enough data for at least sync+sync+len+crc?
	 * If not, let's not waste any time.
	 */
	if (len < 4)
		return (0);

	/* Look for sync byte(s) */
	while (len > 4) {
		if (buf[i] == 0xaa && buf[i+1] == 0xaa && buf[i+2] < 0xaa)
			break;
		i++;
		len--;
	}

#if PKT_DEBUG
	printf("post-look: len=%d, i=%d\n", len, i);
#endif

	/*
	 * If we're out of data, then consume the whole buffer.
	 * We didn't find a sync marker anywhere.
	 *
	 * XXX wrong!
	 */
	if (len == 0) {
#if PKT_DEBUG
		printf("didn't find anything\n");
#endif
		return (i);
	}

	/*
	 * Ok, so there's some data left. We assume it's at least one
	 * sync byte.  So here we don't consume past the start of this
	 * buffer so we can continue appending it with a subsequent read.
	 */
	if (len <= 4)
		return (i);

	/* Grab the length, validate it */
	plen = buf[i+2];
	if (plen >= 0xaa) {
		/* Invalid, consume everything up to here */
		return (i);
	}

	/*
	 * Do we have enough data in the buffer for this length?
	 * If not, consume up to this point.
	 */
	if (len < (plen + 4))
		return (i);

#if PKT_DEBUG
	/* Ok, let's dump out the buffer as-is, incl. start and crc */
	printf("pkt start (ofs %d) (%d bytes):\n", i, plen);
	print_payload(&buf[i], plen + 4);
	printf("pkt end:\n");
#endif

	/* Calculate payload - doesn't take the header into account */
	cksum = mindwave_calc_checksum(&buf[i + 3], plen);

	/* Check if they match! */
	if (cksum == buf[3 + i + plen]) {
		/* Match! Bump up to the parser */
		mindwave_parse_payload(&buf[i+3], plen);
	} else {
		printf("CHECKSUM MISMATCH: cksum: %02x, calc cksum: %02x\n",
		    buf[3 + i + plen] & 0xff,
		    cksum & 0xff);
		print_payload(&buf[i], plen + 4);
	}

	/* And now, consume */
	return (i + plen + 4);
}

int
mindwave_run(struct mindwave_hdl *mw)
{

	int r, i;

	/* XXX check state */

	/*
	 * Assume that all the previous data that can be parsed
	 * has been read out of the incoming buffer.
	 */
#if PKT_DEBUG
	printf("%s: called; fd=%d, offset=%d, len=%d\n", __func__, mw->ms_fd, mw->read_buf.offset, mw->read_buf.len);
#endif

	/* XXX check there is space, error if we don't */
	r = read(mw->ms_fd,
	    mw->read_buf.buf + mw->read_buf.offset,
	    mw->read_buf.len - mw->read_buf.offset);

#if PKT_DEBUG
	printf("%s: read returned %d\n", __func__, r);
#endif

	if (r < 0) {
		return (-1);
	}

	/* EOF */
	if (r == 0)
		return (-1);

	/* Update read buffer size */
	mw->read_buf.offset += r;

#if PKT_DEBUG
	/*
	 * Now, iterate over the buffer and parse whatever data
	 * exists.  If there's a partial buffer afterwards
	 * it's moved to the beginning of the buffer and
	 * read_buf.offset is updated accordingly.
	 */
	for (i = 0; i < mw->read_buf.offset; i++) {
		printf("%02x ", mw->read_buf.buf[i] & 0xff);
	}
	printf("\n");
#endif

	/* XXX not convinced this is correct! */
	while ((r = mindwave_parse_buffer(mw)) > 0) {
		/* Consume data */
#if PKT_DEBUG
		printf("parse consumed %d bytes\n", r);
#endif
		if (r > 0) {
			memmove(mw->read_buf.buf, mw->read_buf.buf + r, mw->read_buf.offset - r);
			mw->read_buf.offset -= r;
		}
	}
#if PKT_DEBUG
	printf("%s: finish!; fd=%d, offset=%d, len=%d\n", __func__, mw->ms_fd, mw->read_buf.offset, mw->read_buf.len);
#endif

	return (0);
}
