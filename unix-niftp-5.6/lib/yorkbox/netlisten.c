/* unix-niftp lib/yorkbox/netlisten.c $Revision: 5.5 $ $Date: 90/08/01 13:39:40 $ */
#include "ftp.h"
#include "stat.h"
#include "infusr.h"
#include <stdio.h>
#include <errno.h>

/* Name clashes --- sigh */
#undef	DEBUG
#undef	ABORTED
#undef	TERMINATED

#define	_NODEF	/* Don't define things, just extern them */
#include <netio.h>        /* ????? */

/*
 * file:
 *			 netopen.c
 *  YORK BOX CODE
 * last changed: 10-jul-85
 *
 * $Log:	netlisten.c,v $
 * Revision 5.5  90/08/01  13:39:40  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:20:30  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:25:03  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:08:30  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/04/24  19:45:45  pb
 * Fix it to work !
 * Sent to wja.
 * 
 * Revision 5.0  87/03/23  03:58:51  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 *
 * This file contains routines to interface to the YORKBOX. They
 * make the network seem like a normal file. Makes the interface nicer
 */

extern TS      *NETTP;	       /* the GLOBAL TS pointer */

/*
 * listen for a call on the host number given as a parameter
 */
extern char     unknown_host;
char *index();

con_listen(str)
char    *str;
{
	register i;
	int	attempt;
	register nodep nptr = &net_io;
	char    *svc[10];
	char	temp_ybts[ENOUGH];
#define	SVCS	((sizeof svc) / (sizeof svc[0]))
#define	SEP	','
	char	*p;
	int	svcp=0;
	TS *tlisten();

	if(net_open) L_WARN_0(L_10, 0, "con_listen called while net_open\n");

	(void) strcpy(t_addr, str);
	(void) strcpy(temp_ybts, str);
	svc[svcp++] = p = temp_ybts;
	while (p = index(p, SEP))
	{	*p++ = '\0';
		if (*p)
		{	if (svcp < (SVCS -2))
				svc[svcp++] = p;
			else L_WARN_1(L_GENERAL, 0,
				"too many YBTSes in %s\n", str);
		}
	}
	svc[svcp] = NULL;
	nptr->read_count =0;
	starttimer(60*60);	/* re-open log, clear jams, etc ... */
	alarm(60*60);		/* TEMP HACK til start timer is fixed */
	for (attempt=10; !(NETTP = tlisten (svc, "janet")); attempt--)
		if (errno != EBADF)
		{
			if (attempt >= 0)
				L_WARN_4(L_GENERAL, L_TIME,
					"Netlisten (%d) failed (%d/%d)%s\n",
					attempt, errno, terrno, (attempt == 0) ?
					" -- no more logging" : "");
			stat_state(S_FAILISTEN);
			sleep ((attempt < 0) ? 60 * 2 : 10);
		}

	if (attempt < 0) L_WARN_1(L_GENERAL, L_TIME,
		"Call opened OK after %d unlogged failures\n", -attempt);

	starttimer(60*11);
	alarm(11*60);		/* TEMP HACK til start timer is fixed */
	L_LOG_2(L_FULL_ADDR, L_DATE | L_TIME, 
		"Netlisten open to %s for %s\n", tdte(NETTP), str);
	else L_LOG_0(L_GENERAL, L_DATE | L_TIME, "call received\n");

	/*
	 * try to find out who called us.
	 */
	{	char dtecalling[ENOUGH];
		sprintf(dtecalling, "%s/%s", tdte(NETTP), tcalling(NETTP));
		stat_addr(dtecalling);
	}
	stat_state(S_DECODING);
	if(r_addr_trans(tdte(NETTP), tcalling(NETTP), tcalled(NETTP),
						   hostname, argstring) < 0){
		L_WARN_1(L_GENERAL, 0, "can't translate %s\n", tdte(NETTP));
		unknown_host = 1;
	}
	else
		unknown_host = 0;

	nptr->write_count = BLOCKSIZ;
	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	net_open=1;
	return(0);
}

/*
 * search address tables and generate host mnemonic
 * from calling address. name is left in 'nhostname'
 */

extern  char    lnetchar;

#ifndef	NOPREFIXHACK
struct {
	char	netc;
	char	*prefix;
	char	*replace;
} prefixes [] = {
#ifdef	ICDOC
	'l',	(char *) 0,		(char *) 0,
	'l',	"0000",			"0000",
	'j',	"0176.x0.",		"",
	'j',	"000000000001.0176.Y0.","",
#else	ICDOC
	'j',	(char *) 0,		(char *) 0,
	'j',	"0000",			"0000",
#endif	ICDOC
	'j',	"00.",			"",
	'p',	"000040000040.pss.",	"",
	'p',	"000000000040.pss.",	"",
	'p',	"gb-",			"234",
	'p',	"2342",			"2342"
/*	'i',	"",			""	THINK ABOUT THIS ONE	*/
};
#define	PREFIXES	((sizeof prefixes) / (sizeof prefixes[0]))
#endif

r_addr_trans(dte, calling, called, nhostname, hn)
char    *dte, *calling, *called, *nhostname, *hn;
{
	char net = lnetchar;
	char tbuff[ENOUGH];
	int len,i;

	if(hn == NULL){		/* no channel - local transfer */
		(void) strcpy(nhostname, dte);
		return(0);
	}

	strcpy(tbuff, dte);
	strcat(tbuff, ".");
	strcat(tbuff, calling);

#ifndef NOPREFIXHACK
	/* is net set to an existing nwtwork letter ? */
	for (i=1; i<PREFIXES; i++) if (net==prefixes[i].netc) break;
	/* No, so set the `default' */
	if (i >= PREFIXES) net=prefixes[0].netc;

	for (i=1; i<PREFIXES; i++) if (strncmpuc(tbuff, prefixes[i].prefix,
			len=strlen(prefixes[i].prefix)) == 0)
	{	char temp[ENOUGH];
		strcpy(temp, prefixes[i].replace);
		strcat(temp, tbuff+len);
		strcpy(tbuff, temp);
		net = prefixes[i].netc; 
	}
#endif

	L_LOG_4(L_ALWAYS, 0, "TS-Call from <%c.%s.%s=%s>",
			net, dte, calling, tbuff);
	L_LOG_1(L_ALWAYS, L_CONTINUE, " for %s\n", called);
	return(nrs_reverse(net, called, tbuff, nhostname));
}
