#ifndef	__MINDWAVE_HW_H__
#define	__MINDWAVE_HW_H__

/*
 * There's 512 raw samples from the mindwave each second.
 */
#define	MW_RAW_WAVE_SAMPLE_FREQ		512

/* Commands to send to the control device */

/*
 * Connect to a specific headset.
 *
 * <headset id> is a big-endian, 2-byte headset identifier.
 *
 * Format:
 * + 0xC0 <headset id>
 *
 * Response:
 * + headset found
 * + headset not found (after 10 seconds)
 * + request denied (already attempting to connect to a headset)
 */
#define	MW_CMD_CONNECT		0xc0

/*
 * Disconnect from the current headset.
 *
 * Format:
 * + 0xc1
 *
 * Response:
 * + headset disconnected
 * + request denied (headset isn't connected)
 */
#define	MW_CMD_DISCONNECT	0xc1

/*
 * Auto-connect to any unassociated headset.
 *
 * Format:
 *
 * + 0xc2
 *
 * Response:
 * + headset found
 * + headset not found (after 10 seconds)
 * + request denied (already attempting to connect to a headset)
 */
#define	MW_CMD_AUTOCONNECT	0xc2

/*
 * Notifications from firmware
 */

#define	MW_NOTIF_SIGNAL_QUALITY		0x02

#define	MW_NOTIF_ATTENTION		0x04

#define	MW_NOTIF_MEDITATION		0x05

#define	MW_NOTIF_BLINK_STRENGTH		0x16

#define	MW_NOTIF_RAW_WAVE		0x80

#define	MW_NOTIF_ASIC_EEG_POWER		0x83

/*
 * Connected to a headset.
 *
 * format:
 *
 * 0xd0 0x02 <headset id>
 *
 * Where <headset id> is a big-endian two byte headset id.
 */
#define	MW_NOTIF_HEADSET_CONNECTED	0xd0

#define	MW_NOTIF_HEADSET_NOT_FOUND	0xd1

#define	MW_NOTIF_HEADSET_DISCONNECTED	0xd2

/*
 * Last request denied.
 *
 * Format:
 *
 * 0xd3 0x00
 *
 * No extra payload data.
 */
#define	MW_NOTIF_REQUEST_DENIED		0xd3

/*
 * In idle state:
 *
 * One-byte payload; 0x0 is "idle"; 0x1 is "idle and attempting to associate".
 */
#define	MW_NOTIF_STANDBY_IDLE		0xd4

#endif	/* __MINDWAVE_HW_H__ */
