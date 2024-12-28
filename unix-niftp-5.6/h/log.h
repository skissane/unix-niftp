#ifndef	L_ERROR_0

#ifndef	MAILER
#include "opts.h"
#endif	MAILER

extern void log_attr ();

/* Values passed as `flags' */

#define	L_CONTINUE	0x01	/* This is a continuation line */
#define	L_TIME		0x02	/* Log the time as well */
#define	L_DATE		0x04	/* Log the date as well */
#define	L_NOTAG		0x08	/* Omit the tag at the front */
#define	L_NOPID		0x10	/* Omit the pid at the front */
#define	L_FILE		0x20	/* Send to error file */
/* etc ... */

/* Mask bits for ftp_print */
#define	L_ALWAYS	0 || 1	/* Hack to ALWAYS print this */
#define	L_GENERAL	0x00001	/* not yet differentiated */
#define	L_MAJOR_COM	0x00002	/* GO, STOP etc */
#define	L_FULL_ADDR	0x00004	/* print full address */
#define	L_ATTRIB	0x00008	/* print attribute negotiation */
#define	L_10		0x00010	/*  ??  */
#define	L_LOG_EXEC	0x00020	/* Log any exec that is performed */
#define	L_OPENFAIL	0x00040	/* WARN: open failure */
#define	L_80		0x00080	/*  ?? yuck/netsubs.c */
#define	L_BCOUNT	0x00100	/* Log Bcount every 1Kb and marks */
#define	L_LOG_TCC	0x00200	/* Log TCC commands */
#define	L_OMIT_ATTR	0x00400	/* Log omitted attributes */
#define	L_LOG_OOB	0x00800	/* Log out of band packets */
#define	L_LOG_OPEN	0x01000	/* Log the open block */
#define	L_RECV_NET	0x02000	/* Log everything received from the net */
#define	L_SEND_NET	0x04000	/* Log everything sent to the net */
#define	L_DOCKET	0x08000	/* Lock actions on dockets */
#define	L_NOTRETRY	0x10000	/* log hosts not yet due */
#define	L_DEB_ADDR	0x20000 /* debug address traslation */
#define	L_DISKFULL	0x40000	/* log info on testing for full disk */

#ifdef	IGNORE_ERROR
#define	L_ERROR_0(mask, flags, format)
#define	L_ERROR_1(mask, flags, format, a1)
#define	L_ERROR_2(mask, flags, format, a1, a2)
#define	L_ERROR_3(mask, flags, format, a1, a2, a3)
#define	L_ERROR_4(mask, flags, format, a1, a2, a3, a4)
#else	IGNORE_ERROR
#define	L_ERROR_0(mask, flags, format) \
    if (ftp_print & mask) logit(mask, flags, "error", format, 00, 00, 00, 00)
#define	L_ERROR_1(mask, flags, format, a1) \
    if (ftp_print & mask) logit(mask, flags, "error", format, a1, 00, 00, 00)
#define	L_ERROR_2(mask, flags, format, a1, a2) \
    if (ftp_print & mask) logit(mask, flags, "error", format, a1, a2, 00, 00)
#define	L_ERROR_3(mask, flags, format, a1, a2, a3) \
    if (ftp_print & mask) logit(mask, flags, "error", format, a1, a2, a3, 00)
#define	L_ERROR_4(mask, flags, format, a1, a2, a3, a4) \
    if (ftp_print & mask) logit(mask, flags, "error", format, a1, a2, a3, a4)
#endif	IGNORE_ERROR


#ifdef	IGNORE_WARN
#define	L_WARN_0(mask, flags, format)
#define	L_WARN_1(mask, flags, format, a1)
#define	L_WARN_2(mask, flags, format, a1, a2)
#define	L_WARN_3(mask, flags, format, a1, a2, a3)
#define	L_WARN_4(mask, flags, format, a1, a2, a3, a4)
#else	IGNORE_WARN
#define	L_WARN_0(mask, flags, format) \
    if (ftp_print & mask) logit(mask, flags, "warn", format, 00, 00, 00, 00)
#define	L_WARN_1(mask, flags, format, a1) \
    if (ftp_print & mask) logit(mask, flags, "warn", format, a1, 00, 00, 00)
#define	L_WARN_2(mask, flags, format, a1, a2) \
    if (ftp_print & mask) logit(mask, flags, "warn", format, a1, a2, 00, 00)
#define	L_WARN_3(mask, flags, format, a1, a2, a3) \
    if (ftp_print & mask) logit(mask, flags, "warn", format, a1, a2, a3, 00)
#define	L_WARN_4(mask, flags, format, a1, a2, a3, a4) \
    if (ftp_print & mask) logit(mask, flags, "warn", format, a1, a2, a3, a4)
#endif	IGNORE_WARN


#ifdef	IGNORE_ACCNT
#define	L_ACCNT_0(mask, flags, format)
#define	L_ACCNT_1(mask, flags, format, a1)
#define	L_ACCNT_2(mask, flags, format, a1, a2)
#define	L_ACCNT_3(mask, flags, format, a1, a2, a3)
#define	L_ACCNT_4(mask, flags, format, a1, a2, a3, a4)
#else	IGNORE_ACCNT
#define	L_ACCNT_0(mask, flags, format) \
    if (ftp_print & mask) logit(mask, flags, "accnt", format, 00, 00, 00, 00)
#define	L_ACCNT_1(mask, flags, format, a1) \
    if (ftp_print & mask) logit(mask, flags, "accnt", format, a1, 00, 00, 00)
#define	L_ACCNT_2(mask, flags, format, a1, a2) \
    if (ftp_print & mask) logit(mask, flags, "accnt", format, a1, a2, 00, 00)
#define	L_ACCNT_3(mask, flags, format, a1, a2, a3) \
    if (ftp_print & mask) logit(mask, flags, "accnt", format, a1, a2, a3, 00)
#define	L_ACCNT_4(mask, flags, format, a1, a2, a3, a4) \
    if (ftp_print & mask) logit(mask, flags, "accnt", format, a1, a2, a3, a4)
#endif	IGNORE_ACCNT


#ifdef	IGNORE_LOG
#define	L_LOG_0(mask, flags, format)
#define	L_LOG_1(mask, flags, format, a1)
#define	L_LOG_2(mask, flags, format, a1, a2)
#define	L_LOG_3(mask, flags, format, a1, a2, a3)
#define	L_LOG_4(mask, flags, format, a1, a2, a3, a4)
#else	IGNORE_LOG
#define	L_LOG_0(mask, flags, format) \
    if (ftp_print & mask) logit(mask, flags, "log", format, 00, 00, 00, 00)
#define	L_LOG_1(mask, flags, format, a1) \
    if (ftp_print & mask) logit(mask, flags, "log", format, a1, 00, 00, 00)
#define	L_LOG_2(mask, flags, format, a1, a2) \
    if (ftp_print & mask) logit(mask, flags, "log", format, a1, a2, 00, 00)
#define	L_LOG_3(mask, flags, format, a1, a2, a3) \
    if (ftp_print & mask) logit(mask, flags, "log", format, a1, a2, a3, 00)
#define	L_LOG_4(mask, flags, format, a1, a2, a3, a4) \
    if (ftp_print & mask) logit(mask, flags, "log", format, a1, a2, a3, a4)
#endif	IGNORE_LOG


#ifdef	IGNORE_DEBUG
#define	L_DEBUG_0(mask, flags, format)
#define	L_DEBUG_1(mask, flags, format, a1)
#define	L_DEBUG_2(mask, flags, format, a1, a2)
#define	L_DEBUG_3(mask, flags, format, a1, a2, a3)
#define	L_DEBUG_4(mask, flags, format, a1, a2, a3, a4)
#else	IGNORE_DEBUG
#define	L_DEBUG_0(mask, flags, format) \
    if (ftp_print & mask) logit(mask, flags, "debug", format, 00, 00, 00, 00)
#define	L_DEBUG_1(mask, flags, format, a1) \
    if (ftp_print & mask) logit(mask, flags, "debug", format, a1, 00, 00, 00)
#define	L_DEBUG_2(mask, flags, format, a1, a2) \
    if (ftp_print & mask) logit(mask, flags, "debug", format, a1, a2, 00, 00)
#define	L_DEBUG_3(mask, flags, format, a1, a2, a3) \
    if (ftp_print & mask) logit(mask, flags, "debug", format, a1, a2, a3, 00)
#define	L_DEBUG_4(mask, flags, format, a1, a2, a3, a4) \
    if (ftp_print & mask) logit(mask, flags, "debug", format, a1, a2, a3, a4)
#endif	IGNORE_DEBUG

#endif	L_ERROR_0
