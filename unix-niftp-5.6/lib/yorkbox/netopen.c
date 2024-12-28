/* unix-niftp lib/yorkbox/netopen.c $Revision: 5.5 $ $Date: 90/08/01 13:39:42 $ */
#include "ftp.h"
#include "infusr.h"
#include <stdio.h>

/* Name clashes --- sigh */
#undef	DEBUG
#undef	ABORTED
#undef	TERMINATED

#define	_NODEF	/* Don't define things, just extern them */
#include "netio.h"
#define	fac_revchg_flag	1	/* tdirs.h */

/*
 * file:
 *			 netopen.c
 *  YORK BOX CODE
 * last changed: 10-jul-85
 * $Log:	netopen.c,v $
 * Revision 5.5  90/08/01  13:39:42  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:20:26  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:35:55  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.3  87/12/09  16:36:00  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:08:35  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:01:17  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:58:52  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 *
 * This file contains routines to interface to the YORKBOX. They
 * make the network seem like a normal file. Makes the interface nicer
 */


extern TS      *NETTP;	       /* the GLOBAL TS pointer */

#ifndef	SMALL_PROC
struct {
	int	err;
	char	*msg
} yberrs[] = {
    {	TACCERR, 	"access to network barred",	},
    {	TNOFUNDS,	"no funds left to make charged call",},
    {	TADRER,		"invalid network address",	},
    {	TCALREJ,	"TS connection refused",	},
    {	TNOCHAN,	"network inaccesible",		},
    {	TNETDWN,	"network down",			},
    {	TTIMOUT,	"call request timed out",	},
    {	TNOBLK,		"no local control blocks",	}
};
#define	YBERRS ((sizeof yberrs) / (sizeof yberrs[0]))
#endif	SMALL_PROC

/* open a connection to host str and fill in nptr appropriately */

con_open(str)
char    *str;
{
	register i;
	register nodep nptr = &net_io;
	char    *top[2];
	char    fromdte[ENOUGH];
	int     facilities[3];
	TS      *topen();

	if(net_open) L_WARN_0(L_10, 0, "con_open called with net_open\n");

	facilities[0] = (tab.t_flags & T_TYPE) == T_MAIL ? 'M' :
#ifdef	NEWS
			(tab.t_flags & T_TYPE) == T_NEWS ? 'N' :
#endif	NEWS
#ifdef	JTMP
			(tab.t_flags & T_TYPE) == T_JTMP ? 'J' :
#endif	JTMP
							   'F';
	/*      should be something like this:-
	facilities[1] = tab.t_uid;
	* actually is for now:- */
	facilities[1] = 0;
	facilities[2] = 0;
	if (tab.t_flags & REVERSE_CHARGING) facilities[2] |= fac_revchg_flag;
/*	if (tab.t_flags & ??) facilities[2] |= fac_nofast_flag; */

	top[0] = fromdte;
	top[1] = t_addr;
	if(i=adr_trans(str, network, top) ){
		/* translate to get network too */
		L_WARN_1(L_GENERAL, 0, "Address trans error %d\n",i);
		return(-1);
	}

	L_LOG_2(L_FULL_ADDR, 0, "Full address:- %s (from %s)\n",
		hide_pss_pw(t_addr), fromdte);

	nptr->read_count = 0;
	starttimer(2*60);		/* start the timeout system */
	alarm(2*60);		/* TEMP HACK til start timer is fixed */
	NETTP = topen(t_addr, fromdte, facilities);
	if( NETTP == 0) {
		if(ftp_print & L_GENERAL)
		{
			char *reason = "typical!";
#ifdef	YBERRS
			int i;
			for (i=0; i<YBERRS; i++) if (terrno == yberrs[i].err)
			{	reason = yberrs[i].msg;
				break;
			}
#endif	YBERRS
			L_WARN_2(L_OPENFAIL, 0, "Netopen to %s failed -- %s\n",
				hide_pss_pw(t_addr), reason); /*failed*/
		}
		return(1);
	}
	starttimer(11*60);		/* give it a chance now .... */
	nptr->write_count = BLOCKSIZ;
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	net_open = 1;                   /* say we are open */
	return(0);
}
