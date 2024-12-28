/* unix-niftp h/retry.h $Revision: 5.5 $ $Date: 90/08/01 13:33:29 $ */
/*
 * structures needed for retries when useing NRS type info for doing retries
 *
 * $Log:	retry.h,v $
 * Revision 5.5  90/08/01  13:33:29  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:41:32  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:26:08  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "../h/hcontrol.h"

#define BCONTROL "bcont"
#define BFSIZE  64

struct  backoff {
	char    b_host[BFSIZE]; /* name of host */
	struct  hcontrol b_hc;  /* host control information */
				/* the h_ntrans field is no longer used */
};

/*
 * the backoff file structure consists of the following structure
 * there is one backoff file for every queue ( in the queue directory )
 * the name of the file starts with a 'b'
 */

#define MAXBHOSTS       100

struct  bfile   {
	int     b_nhosts;    /* when started this file contains 100 entrys */
	struct  backoff b_hosts[MAXBHOSTS];  /* 100 hosts should be enough */
};
