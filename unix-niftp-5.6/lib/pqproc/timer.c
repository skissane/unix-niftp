/* unix-niftp lib/pqproc/timer.c $Revision: 5.6 $ $Date: 1991/06/07 17:01:27 $ */
#include  "ftp.h"

/* file:  timer.c
 * last changed: 15-aug-83
 * $Log: timer.c,v $
 * Revision 5.6  1991/06/07  17:01:27  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:37:40  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:53:45  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:50:36  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

/*
 * routines to deal with timeouts - if no data transfered in time secs
 * give up.
 */


clockquack()    /* called when got signal 14 ( timer signal ) */
{
	L_WARN_0(L_FULL_ADDR, L_TIME, "Alarmed\n");
	killoff(0,TIMEOUTSTATE,0);
}

starttimer(n) /* signal to say we want to be woken up in time secs */
int n;
{
	(void) signal(SIGALRM, clockquack);
	time_out = n;       /* get it right !! */
	alarm(time_out);
}

/* stop the clock quacking */

stoptimer()
{
	(void) signal(SIGALRM, SIG_IGN);
	(void) alarm(0);
}

/* postpone the evil moment a bit (11 minutes - bit more than is needed ) */

clocktimer()
{
	(void) alarm(time_out);
}
