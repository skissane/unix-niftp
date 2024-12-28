#ifndef	lint			/* unix-niftp lib/x25b/netlieten.c */
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/x25b/netlisten.c,v 5.5 90/08/01 13:39:15 pb Exp $";
#endif	lint

/*
 * file: netlisten.c
 *
 *
 *	===============================================================
 *	===============================================================
 *
 *		NO-OP
 *
 *	===============================================================
 *	===============================================================
 *
 * Piete Brooks <pb@cl.cam.ac.uk>
 *
 * $Log:	netlisten.c,v $
 * Revision 5.5  90/08/01  13:39:15  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:21:44  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:34:54  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0  88/01/28  06:23:25  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 *
 * This file contains routines to interface to the x25bridge libraries.
 * They make the network seem like a normal file. Makes the interface nicer
 */

#include "ftp.h"
#include "stat.h"
#include "infusr.h"
#include <stdio.h>
#include <errno.h>

/*
 * listen for a call on the host number given as a parameter
 *
 * Actually it's too late, as the call is already opened by the yb daemon.
 */
extern char	unknown_host;
void sigurg();
extern fid;
#ifndef	Q_BIT
#define	Q_BIT 3
#endif	Q_BIT
static char	qbit	= 1 << Q_BIT;
static char	accept[] = { 0x11, 0x80 };


con_listen(str)
char    *str;
{
	char	*dte = getenv("X25DTE");
	char	*called = getenv("YBTSCALLED");
	char	*calling = getenv("YBTSTEXT");	/* Strange name !! */

	fid = 4;

	(void) strcpy(t_addr, called);

	if(net_open) L_WARN_0(L_10, 0, "con_listen called while net_open\n");

	{	int sendrc = x25b_write_data(fid, &qbit,accept,sizeof accept);
		L_LOG_2(L_GENERAL, 0, "Accept gave %d (%d)\n",
			sendrc, sizeof accept);
	}

	L_LOG_4(L_FULL_ADDR, L_DATE | L_TIME, 
	   "Netlisten open to %s.%s for %s(%s)\n", dte, calling, str, called);
	else L_LOG_0(L_GENERAL, L_DATE | L_TIME, "call received\n");

	{	char dte_calling[ENOUGH];
		sprintf(dte_calling, "%s.%s", dte, calling);
		stat_addr(dte_calling);
	}
	stat_state(S_DECODING);
	if(r_addr_trans(dte, calling, called, hostname, argstring) < 0) {
		L_WARN_2(L_GENERAL, 0, "can't translate %s.%s\n", dte, calling);
		unknown_host = 1;
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

	if(hn == NULL){		/* no channel - local transfer */
		(void) strcpy(nhostname, dte);
		L_WARN_1(L_GENERAL, 0, "Null channel (use %s ??)\n", dte);
		/* return(0);	*/
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
