#ifndef	lint			/* unix-niftp lib/sun/netlieten.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/sun/RCS/netlisten.c,v 5.6.1.7 1993/05/10 13:58:27 pb Rel $";
#endif	lint

/*
 * file: netlisten.c
 *
 *
 *	===============================================================
 *	===============================================================
 *
 *		INITIAL PARTIAL IMPLEMENTATION ( see ??'s )
 *
 *	===============================================================
 *	===============================================================
 *
 * Piete Brooks <pb@cl.cam.ac.uk>
 *
 * $Log: netlisten.c,v $
 * Revision 5.6.1.7  1993/05/10  13:58:27  pb
 * Distribution of Apr93SunybytsdPPLDYbAANSICC: Sun YBTSD + PP LD_ + YuckBucked ANSI CC preliminary HACK
 *
 * Revision 5.6.1.6  1993/01/10  07:12:03  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:02:15  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.1  88/10/07  17:26:30  pb
 * checked in with -k by pb at 89.09.29.10.21.38.
 * 
 * Revision 5.1  88/10/07  17:26:30  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:59:57  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:54:36  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 *
 * This file contains routines to interface to the SUNlink libraries.
 * They make the network seem like a normal file. Makes the interface nicer
 */

#include "ftp.h"
#include "stat.h"
#include "infusr.h"
#include <stdio.h>
#include <errno.h>
#include <sys/mbuf.h>
#include <sundev/syncstat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netx25/x25_pk.h>
#include <netx25/x25_ioctl.h>

/* This allows "sloppy" sites to HACK in a "-DHOST_UNKNOWN=0" or some suxh */
#ifndef	HOST_UNKNOWN
#define	HOST_UNKNOWN 1
#endif

/*
 * listen for a call on the host number given as a parameter
 *
 * Actually it's too late, as the call is already opened by the yb daemon.
 */
extern char	unknown_host;
extern int	is_cons;
void sigurg();
extern fid;
extern q_daemon;
extern char *strcat ();

static unsigned char	accept[] = { 0x11, 0x80 };
static char sun_cons_pref[] = "DCC.";
#define	SCPLEN (sizeof(sun_cons_pref) -1)

con_listen(str)
char    *str;
{
	char	*dte = getenv("X25DTE");
	char	*called = getenv("YBTSCALLED");
	char	*calling = getenv("YBTSTEXT");	/* Strange name !! */
	char	*called_aef = getenv("CALLED_AEF");
	char	*calling_aef = getenv("CALLING_AEF");
	char	*x25_fd = getenv("X25_FD");

	is_cons = called_aef && calling_aef && *called_aef && *calling_aef;

	/* SUN's ybtsd doesn't set CALL*_AEF, but a CONS call has:
	 *	X25DTE=<MAC address>
	 *	YBTSCALLED=ftp (or similar)
	 *	YBTSTEXT=<called NSAP: DCC.3882611...>
	 * T.D.Lee@durham.ac.uk 93/4/6
	 */
	if (!is_cons && strncasecmp(sun_cons_pref, calling, SCPLEN) == 0 &&
	    strspn(calling + SCPLEN, "0123456789") == strlen(calling + SCPLEN))
	{	L_LOG_1(L_10, 0,
		"called prefixed by `%s' so assume CONS call to Sun's ybtsd\n",
			sun_cons_pref);
		called_aef = calling + SCPLEN;	/* Yes! It's horrible. */
		calling_aef = "Unknown.Calling.NSAP";
		is_cons++;
	}

	fid = 4;
	if (x25_fd && '0' <= *x25_fd && *x25_fd <= '9')
	{	int x25_fid = atoi(x25_fd);
		if (x25_fid != fid) L_LOG_3(L_GENERAL, 0,
			"X25_FD=%s -> %d (was %d)\n", x25_fd, x25_fid, fid);
		fid = x25_fid;
		/* if (is_cons && fid == 4) fid = 0; */
	}

	(void) strcpy(t_addr, (called) ? called : "");

	if (net_open) L_WARN_0(L_10, 0, "con_listen called while net_open\n");
	if (!q_daemon) {
		q_daemon = 1;
		L_WARN_0(L_GENERAL, 0, "q_daemon was unset\n");
	}

	/* Set the process group for the sockets */
	{	int proc_group = getpid();
		if (ioctl(fid, SIOCSPGRP, &proc_group) < 0)
			L_WARN_3(L_GENERAL, 0,
			"Failed to set process group for fd %d to %d (%d)\n",
				fid, proc_group, errno);
	}

	if (signal(SIGURG, sigurg) == BADSIG)
		L_WARN_1(L_GENERAL, 0, "Failed to set sigurg signal (%d)\n",
					errno);


	/* By default the stupid sun THROWS AWAY any data in the
	 * current packet is the buffer isn't big enough !!
	 */
	{	int one = 1;
		if (ioctl(fid, X25_RECORD_SIZE, &one))
			L_WARN_0(L_GENERAL, 0, "Failed to set record size\n");
	}

	/* STUPID sun get the "give me the bits byte" wrong.
	 * You have to request the byte BEFORE the data ARRIVES,
	 * NOT before you READ the data ...
	 */
	set_read(fid);

	if (!is_cons)
	{	int	send_type = (1 << Q_BIT);
		int	sendrc;
		if (ioctl(fid, X25_SEND_TYPE, &send_type))
			L_WARN_1(L_GENERAL, 0, "Failed to set QUAL bit(%d)\n",
							errno);
		sendrc = send(fid, accept, sizeof accept, 0);
		L_LOG_2(L_GENERAL, 0, "Accept gave %d (%d)\n",
			sendrc, sizeof accept);
	}

	if (!is_cons)
	{	char dte_calling[ENOUGH];
		sprintf(dte_calling, "%s.%s", dte, calling);
		stat_addr(dte_calling);
		L_LOG_4(L_FULL_ADDR, L_DATE | L_TIME, 
			"Netlisten YBTS open from %s.%s for %s(%s)\n",
			dte, calling, str, called);
		else L_LOG_0(L_GENERAL, L_DATE | L_TIME, "call received\n");
	}
	else
	{	char dte_calling[ENOUGH];
		sprintf(dte_calling, "%s-%s", calling_aef, dte);
		stat_addr(dte_calling);
		L_LOG_3(L_FULL_ADDR, L_DATE | L_TIME, 
			"Netlisten CONS open from %s (%s) for %s\n",
	  		 calling_aef, dte, called_aef);
		else L_LOG_0(L_GENERAL, L_DATE | L_TIME, "call received\n");
	}

	stat_state(S_DECODING);
	if(r_addr_trans(dte,
		(is_cons) ? calling_aef : calling,
		(is_cons) ? called_aef : called,
		hostname, argstring) < 0) {
		L_WARN_2(L_GENERAL, 0, "can't translate %s.%s\n", dte,
				(is_cons) ? calling_aef : calling);
		unknown_host = HOST_UNKNOWN;
	}
	else	unknown_host = 0;

	net_io.write_count = BLOCKSIZ;
	net_io.read_buffer = net_read_buffer;
	net_io.write_buffer = net_write_buffer;
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

	/* ????? what on earth is hn for ?????? */
	if(hn == NULL){		/* no channel - local transfer */
		(void) strcpy(nhostname, dte);
		/* L_WARN_1(L_GENERAL, 0, "Null channel (use %s ??)\n", dte);
		 * return(0);	*/
	}

	if (is_cons)
	{	strcpy(tbuff, calling);
		strcat(tbuff, ".");
		strcat(tbuff, dte);
	}
	else
	{	strcpy(tbuff, dte);
		strcat(tbuff, ".");
		strcat(tbuff, calling);
	}

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

	if (is_cons)
	{	L_LOG_4(L_ALWAYS, 0, "TS-Call from <[%c=]o.%s.%s=%s>",
			net, dte, calling, tbuff);
		L_LOG_1(L_ALWAYS, L_CONTINUE, " for %s\n", called);
	}
	else
	{	L_LOG_4(L_ALWAYS, 0, "TS-Call from <%c.%s.%s=%s>",
			net, dte, calling, tbuff);
		L_LOG_1(L_ALWAYS, L_CONTINUE, " for %s\n", called);
	}
	return(nrs_reverse_c(net, -2, called, tbuff, nhostname));
}
