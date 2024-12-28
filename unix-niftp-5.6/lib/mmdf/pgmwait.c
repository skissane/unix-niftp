/* unix-niftp lib/mmdf/pgmwait.c $Revision: 5.5 $ $Date: 90/08/01 13:36:23 $ */
#include "util.h"
/*
 * file pgmwait.c
 * last changed 25-apr-83
 *	 extracted from nexec.c for use with ml_send.c
 * $Log:	pgmwait.c,v $
 * Revision 5.5  90/08/01  13:36:23  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:48:50  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:46:00  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/


pgmwait (childid)
    int childid;                  /* process id of child to collect     */
{
    int status;
    register int retval;

    while ((retval = wait (&status)) != childid)
	if (retval == NOTOK)
	    return (NOTOK);

    return (status);
}
