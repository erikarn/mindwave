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
	const char *ms_sport;

	/* Serial port FD */
	int ms_fd;

	/* Current state */
	mindwave_state_t ms_state;
};

#endif	/* __MINDWAVE_H__ */
