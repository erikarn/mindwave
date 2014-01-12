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

struct mindwave_hdl;

struct mindwave_raw_sample;

typedef void mw_attention_cb(struct mindwave_hdl *mw, void *cbdata, uint32_t attention);
typedef void mw_meditation_cb(struct mindwave_hdl *mw, void *cbdata, uint32_t meditation);
typedef void mw_quality_cb(struct mindwave_hdl *mw, void *cbdata, uint8_t quality);

struct mindwave_raw_sample {
	struct timeval tv;
	int16_t sample;
	uint32_t seqno;
};

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

	/* Raw sampling data */
	struct {
		struct mindwave_raw_sample *s;
		int len;
		int offset;
		uint32_t cur_seq;
	} raw_samples;

	/* Callbacks */
	struct {
		void *cbdata;
		mw_attention_cb *ma;
		mw_meditation_cb *mm;
		mw_quality_cb *mq;
	} cb;
};

extern	struct mindwave_hdl * mindwave_new(void);

/* XXX until I get poll / kqueue working */
extern	int mindwave_run(struct mindwave_hdl *mw);

extern	int mindwave_set_serial(struct mindwave_hdl *mw, const char *sport);
extern	int mindwave_open(struct mindwave_hdl *mw);

extern	int mindwave_send_disconnect(struct mindwave_hdl *mw);
extern	int mindwave_connect_headset(struct mindwave_hdl *mw, uint16_t headset_id);
extern	int mindwave_connect_headset_any(struct mindwave_hdl *mw);

extern	void mindwave_setup_cb(struct mindwave_hdl *mw, void *cbdata,
	    mw_attention_cb *ma, mw_meditation_cb *mm, mw_quality_cb *mq);

#endif	/* __MINDWAVE_H__ */
