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

#include "mindwave_hw.h"
#include "mindwave.h"

#define	SER_PORT		"/dev/cuaU0"

int
main(int argc, const char *argv[])
{
	struct mindwave_hdl *mw;

	mw = mindwave_new();
	if (mw == NULL)
		err(1, "mindwave_new");

	if (mindwave_set_serial(mw, SER_PORT) != 0)
		err(1, "mindwave_set_serial");

	if (mindwave_open(mw) != 0)
		err(1, "mindwave_open");

	/* Disconnect from any existing headset */
	sleep(1);
	if (mindwave_send_disconnect(mw) < 0)
		err(1, "mindwave_disconnect");

	/* Connect to a specific headset */
	sleep(1);
	if (mindwave_connect_headset(mw, 0x871f) < 0)
		err(1, "mindwave_connect_headset");

	while (1) {
		if (mindwave_run(mw) < 0)
			break;
	}

	exit(0);
}
