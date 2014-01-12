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
#include <poll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <err.h>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <fftw3.h>

#include "mindwave_hw.h"
#include "mindwave.h"

#define	SER_PORT		"/dev/cuaU0"

struct mw_app {
	int quality;
	int attention;
	int meditation;
	struct mindwave_hdl *mw;
	fftw_complex *fft_in;
	fftw_complex *fft_out;
	fftw_plan fft_p;
};

/* SDL bits */

static void
quit_tutorial(int code)
{
	SDL_Quit();
	exit(code);
}

static void
process_events(void)
{
	SDL_Event event;

	while(SDL_PollEvent(&event)) {

		switch(event.type) {
//		case SDL_KEYDOWN:
//			/* Handle key presses. */
//			handle_key_down( &event.key.keysym );
//			break;
		case SDL_QUIT:
			/* Handle quit requests (like Ctrl-c). */
			quit_tutorial( 0 );
			break;
		}
	}
}

static void
update_fft_in(struct mw_app *app)
{
	int curofs;
	int size;
	int i;

	curofs = app->mw->raw_samples.offset;
	size = app->mw->raw_samples.len;

	/*
	 * Fill out the fft_in array with our current sample data.
	 */
	for (i = 0; i < size; i++) {
		/* XXX do I normalise this? */
		app->fft_in[i][0] =
		     (float) app->mw->raw_samples.s[curofs].sample;

#if 0
		/* Clip? */
		if (app->fft_in[i][0] > 512.0)
			app->fft_in[i][0] = 512.0;
		if (app->fft_in[i][0] < -512.0)
			app->fft_in[i][0] = -512.0;
#endif

		app->fft_in[i][1] = 0.0;
		curofs = (curofs + 1) % size;
	}
}

/*
 * Draw just the raw signal line - signal in the time domain.
 */
static void
draw_signal_line(struct mw_app *app)
{
	int x, y;
	int curofs;
	int size;

	/*
	 * Plot a line series for the last 320 elements,
	 * two pixel wide (640 pixels.)
	 */
	curofs = app->mw->raw_samples.offset - 1;
	size = app->mw->raw_samples.len;
	if (curofs < 0)
		curofs = size - 1;

	glBegin(GL_LINE_STRIP);
	glColor3f(1, 1, 0);
	for (x = 0; x < 320; x++) {
		/*
		 * Y axis is 480 high, so let's split it into
		 * +/- 240.  The dynamic range of the raw
		 * values is signed 16-bit value, so scale that.
		 */
		y = app->mw->raw_samples.s[curofs].sample;
		y = y * 240 / 1024;

		/* Clip */
		if (y < -240)
			y = -240;
		else if (y > 240)
			y = 240;

		/* Set offset to middle of the screen */
		y += 240;

		/* Go backwards in samples */
		curofs--;
		if (curofs < 0)
			curofs = size - 1;
		glVertex3f(x * 2, y, 0);
	}
	glEnd();
}

/*
 * Draw the FFT data out.
 */
static void
draw_fft_data(struct mw_app *app)
{
	int size, i;
	float x, y;

	/*
	 * Let's map the +ve FFT frequency domain point
	 * space into our width (640 pixels.)
	 *
	 * Yes, some sub-pixel rendering will likely occur.
	 */
	glBegin(GL_LINE_STRIP);
	glColor3f(0, 1, 1);
	size = app->mw->raw_samples.len;
	for (i = 0; i < size / 2; i++) {
		/* XXX ignore complex? */
		x = ((float) i * (float) 640 * (float) 2) / (float) size;
		y = app->fft_out[i][0] / 128.0;
		/* centre it on the midpoint */
		y += 240;
		glVertex3f(x, y, 0);
	}
	glEnd();
}

static void
draw_screen(struct mw_app *app)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    /*
     * let's draw three strips, for quality, attention and meditation
     */

    /* Quality */
    glBegin(GL_QUADS);
     glColor3f(1, 0, 0);
     glVertex3f(0, 480, 0);
     glVertex3f(50, 480, 0);
     glVertex3f(50, 480 - app->quality, 0);
     glVertex3f(0, 480 - app->quality, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
     glColor3f(1, 1, 1);
     glVertex3f(0, 480, 0);
     glVertex3f(50, 480, 0);
     glVertex3f(50, 480 - 200, 0);
     glVertex3f(0, 480 - 200, 0);
     glVertex3f(0, 480, 0);
    glEnd();

    /* Meditation */
    glBegin(GL_QUADS);
     glColor3f(0, 1, 0);
     glVertex3f(100, 480, 0);
     glVertex3f(150, 480, 0);
     glVertex3f(150, 480 - app->meditation, 0);
     glVertex3f(100, 480 - app->meditation, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
     glColor3f(1, 1, 1);
     glVertex3f(100, 480, 0);
     glVertex3f(150, 480, 0);
     glVertex3f(150, 480 - 100, 0);
     glVertex3f(100, 480 - 100, 0);
     glVertex3f(100, 480, 0);
    glEnd();

    /* Attention */
    glBegin(GL_QUADS);
     glColor3f(0, 0, 1);
     glVertex3f(200, 480, 0);
     glVertex3f(250, 480, 0);
     glVertex3f(250, 480 - app->attention, 0);
     glVertex3f(200, 480 - app->attention, 0);
    glEnd();
    glBegin(GL_LINE_STRIP);
     glColor3f(1, 1, 1);
     glVertex3f(200, 480, 0);
     glVertex3f(250, 480, 0);
     glVertex3f(250, 480 - 100, 0);
     glVertex3f(200, 480 - 100, 0);
     glVertex3f(200, 480, 0);
    glEnd();

	draw_signal_line(app);
	draw_fft_data(app);

	/* This waits for vertical refresh before flipping, so it sleeps */
	SDL_GL_SwapBuffers();
}

static int
scr_init(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL_Init failed: %s\n", SDL_GetError());
	return 0;
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,            5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,          5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,           5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,          16);

	if (SDL_SetVideoMode(640, 480, 15,
	    SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL) == NULL) {
		printf("SDL_SetVideoMode failed: %s\n", SDL_GetError());
		return 0;
	}

	glClearColor(0, 0, 0, 0);

	glViewport(0, 0, 640, 480);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, 640, 480, 0, 1, -1);

	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();

	return 1;
}

static void
mw_app_attention_cb(struct mindwave_hdl *mw, void *cbdata, uint32_t val)
{
	struct mw_app *app = cbdata;

	app->attention = val;
}

static void
mw_app_meditation_cb(struct mindwave_hdl *mw, void *cbdata, uint32_t val)
{
	struct mw_app *app = cbdata;

	app->meditation = val;
}

static void
mw_app_quality_cb(struct mindwave_hdl *mw, void *cbdata, uint8_t val)
{
	struct mw_app *app = cbdata;

	app->quality = val;
}

static void
mw_poll_check(struct mindwave_hdl *mw)
{
	struct pollfd pollfds[2];
	int r, n;

	bzero(pollfds, sizeof(pollfds));
	pollfds[0].fd = mw->ms_fd;
	pollfds[0].events = POLLRDNORM | POLLIN;

	r = poll(pollfds, 1, 0);
	if (r == 0)
		return;

	if (r < 0) {
		warn("%s: poll", __func__);
		return;
	}

	/* XXX validate */
	(void) mindwave_run(mw);
}

int
main(int argc, const char *argv[])
{
	struct mw_app app;
	struct pollfd pollfds[2];
	int r, n;

	bzero(&app, sizeof(app));

	app.mw = mindwave_new();
	if (app.mw == NULL)
		err(1, "mindwave_new");

	if (mindwave_set_serial(app.mw, SER_PORT) != 0)
		err(1, "mindwave_set_serial");

	/* Setup callbacks */
	mindwave_setup_cb(app.mw, &app, mw_app_attention_cb, mw_app_meditation_cb, mw_app_quality_cb);

	/*
	 * Now we've set it up, let's fetch the sample size
	 * and setup the fft plan.
	 */
	app.fft_in = fftw_malloc(sizeof(fftw_complex) * app.mw->raw_samples.len);
	app.fft_out = fftw_malloc(sizeof(fftw_complex) * app.mw->raw_samples.len);
	app.fft_p = fftw_plan_dft_1d(app.mw->raw_samples.len, app.fft_in, app.fft_out, FFTW_FORWARD, FFTW_ESTIMATE);

	if (mindwave_open(app.mw) != 0)
		err(1, "mindwave_open");

	/* Disconnect from any existing headset */
	sleep(1);
	if (mindwave_send_disconnect(app.mw) < 0)
		err(1, "mindwave_disconnect");

	/* Connect to a specific headset */
	sleep(1);
	if (mindwave_connect_headset(app.mw, 0x871f) < 0)
		err(1, "mindwave_connect_headset");

	if (scr_init() == 0)
		exit(127);

	/*
	 * Poll.
	 */
	while (1) {
		/* Run mindwave IO loop if needed */
		mw_poll_check(app.mw);
		/* Process incoming events. */
		process_events();

		/* Update incoming FFT */
		update_fft_in(&app);

		/* Run FFT */
		fftw_execute(app.fft_p);

		/* Draw the screen. */
		draw_screen(&app);
	}

	exit(0);
}
