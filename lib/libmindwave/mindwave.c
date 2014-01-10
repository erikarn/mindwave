#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/types.h>

#include "mindwave_hw.h"
#include "mindwave.h"

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

int
mindwave_run(struct mindwave_hdl *mw)
{

	int r;

	/* XXX check state */

	/*
	 * Assume that all the previous data that can be parsed
	 * has been read out of the incoming buffer.
	 */

	/* XXX check there is space, error if we don't */
	r = read(mw->ms_fd,
	    mw->read_buf.buf + mw->read_buf.offset,
	    mw->read_buf.len - mw->read_buf.offset);
	if (r < 0) {
		return (-1);
	}

	/* EOF */
	if (r == 0)
		return (-1);

	/* Update read buffer size */
	mw->read_buf.offset += r;

	/*
	 * Now, iterate over the buffer and parse whatever data
	 * exists.  If there's a partial buffer afterwards
	 * it's moved to the beginning of the buffer and
	 * read_buf.offset is updated accordingly.
	 */

	return (0);
}
