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

#include "mindwave_hw.h"
#include "mindwave.h"

#define	SER_PORT		"/dev/cuaU0"

struct mw_app {
	int quality;
	int attention;
	int meditation;
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
	struct mindwave_hdl *mw;
	struct mw_app app;
	struct pollfd pollfds[2];
	int r, n;

	bzero(&app, sizeof(app));

	mw = mindwave_new();
	if (mw == NULL)
		err(1, "mindwave_new");

	if (mindwave_set_serial(mw, SER_PORT) != 0)
		err(1, "mindwave_set_serial");

	/* Setup callbacks */
	mindwave_setup_cb(mw, &app, mw_app_attention_cb, mw_app_meditation_cb, mw_app_quality_cb);

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

	if (scr_init() == 0)
		exit(127);

	/*
	 * Poll.
	 */
	while (1) {
		/* Run mindwave IO loop if needed */
		mw_poll_check(mw);
		/* Process incoming events. */
		process_events();
		/* Draw the screen. */
		draw_screen(&app);
	}

	exit(0);
}
