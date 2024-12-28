/* unix-niftp h/hcontrol.h $Revision: 5.5 $ $Date: 90/08/01 13:33:08 $ */
/*
 * $Log:	hcontrol.h,v $
 * Revision 5.5  90/08/01  13:33:08  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:41:11  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:25:49  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * This is the structure for the host status file. The name of this file
 * is put into the queue.
 *      Its primary use is to control retry attempts for the given host.
 *      This is only accessed For each new host. Or if the connection dies.
 */

struct  hcontrol {
	long    h_nextattempt;  /* time of next attempt if last failed */
	int     h_retries;      /* numb of retries that should be attempted */
	int     h_nback;        /* numb of attempts to open */
	};

#define D_SLEEPTIME     (5*60)  /* how long to sleep if host down */
#define B_SLEEPTIME     20      /* how long to sleep if host busy */

#define NRETRIES        100     /* standard number of retries allowed */
#define QRETRIES	NRETRIES
#define QTIMEOUT	(60L*60*NRETRIES)

#define NMAXTSTRANS	50	/* max number of trans /TS cons */

#define H_SIZE          sizeof(struct hcontrol)
