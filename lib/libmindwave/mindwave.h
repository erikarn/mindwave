#ifndef	__MINDWAVE_H__
#define	__MINDWAVE_H__

/*
 * This covers the current state of the mindwave dongle.
 */
typedef enum {
	MS_NONE,
	MS_IDLE,
	MS_SCANNING,
	MS_RUNNING
} mindwave_state_t;

struct mindwave_hdl {
	/* Serial port path */
	char *ms_sport;

	/* Serial port FD */
	int ms_fd;

	/* Current state */
	mindwave_state_t ms_state;

	struct {
		uint16_t desired_headset_id;
		uint16_t connected_headset_id;
		int use_desired;
	} ms_headset;

	/* Serial buffer */
	struct {
		char *buf;
		int len;
		int offset;
	} read_buf;

	/* Callbacks */
};

extern	struct mindwave_hdl * mindwave_new(void);

/* XXX until I get poll / kqueue working */
extern	int mindwave_run(struct mindwave_hdl *mw);

extern	int mindwave_set_serial(struct mindwave_hdl *mw, const char *sport);
extern	int mindwave_open(struct mindwave_hdl *mw);

extern	int mindwave_send_disconnect(struct mindwave_hdl *mw);
extern	int mindwave_connect_headset(struct mindwave_hdl *mw, uint16_t headset_id);
extern	int mindwave_connect_headset_any(struct mindwave_hdl *mw);

#endif	/* __MINDWAVE_H__ */
