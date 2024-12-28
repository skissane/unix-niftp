/* unix-niftp lib/dexpand/netlisten.c $Revision: 5.5 $ $Date: 90/08/01 13:33:56 $ */
#include "ftp.h"
#include "stat.h"
#include "infusr.h"
#include <stdio.h>
#include <errno.h>

#include <cci.h>
extern errno;

/*
 * file:
 *			 netlisten.c
 *  Dexpand code
 * last changed: 5 - Jun - 86
 * $Log:	netlisten.c,v $
 * Revision 5.5  90/08/01  13:33:56  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:17:06  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:07:12  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:59:45  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:32:45  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

/*
 * This file contains routines to interface to the Camtec DEXPAND X.25
 * board. They make the network seem like a normal file.
 * Makes the interface nicer
 */

extern  int	circuit;
extern  struct  ccontrolp contp;

/*
 * listen for a call on the host number given as a parameter
 */
extern char     unknown_host;

con_listen(str)
char    *str;
{
	register i;
	register nodep nptr = &net_io;
	int    attempt;
	char    remadr[ENOUGH];
	char    fromdte[ENOUGH];
	int    outer = 0;
	int conn_rc = -1;
	register char   *p;

	if(net_open) L_WARN_0(L_10, 0, "con_listen called while net_open\n");

	for(; outer < 10 && conn_rc < 0; outer++)
	{	if(circuit < 0) for (attempt=10;
			(circuit = ccinit(0, CC_YBTS)) < 0; attempt--)
		{	if (attempt >= 0)
				L_WARN_3(L_GENERAL, L_TIME,
					"Dexpand init (%d, %d) failed%s\n",
					attempt, outer, (attempt == 0) ?
					" -- no more logging" : "");
			stat_state(S_FAILISTEN);
			sleep ((attempt < 0) ? 60 * 2 : 10);
			stat_state(S_LISTEN);
		}

		if (attempt < 0) L_WARN_2(L_GENERAL, L_TIME,
		       "Circuit opened OK after %d unlogged failures on %d\n",
				-attempt, outer);

		nptr->read_count =0;

		contp.c_TSrecv = CRECV;
		contp.c_TSlisten = str;
		contp.c_TSllen = strlen(str);
		contp.c_TSradr = remadr;        /* calling */
		contp.c_TSralen = ENOUGH;
		contp.c_TSlname = fromdte;      /* called YBTS */
		contp.c_TSlnlen = ENOUGH;

		starttimer();

		for (attempt=9; attempt > -3 &&
			(conn_rc=ccontrol(circuit, CCONNECT, &contp))<=0;
			attempt--)
		{	int reason = errno;
			if (attempt >= 0) L_WARN_4(L_GENERAL, L_TIME,
				"Netlisten (%d, %d) failed %d%s\n",
				attempt, outer, reason, (attempt == 0) ?
				" -- no more logging" : "");
			stat_state(S_FAILISTEN);
			sleep ((attempt < 0) ? 60 : 6);
			stat_state(S_LISTEN);
			if (reason == ENODEV && attempt < 5)	break;
		}

		if (conn_rc <= 0)
		{	L_WARN_0(L_GENERAL, L_TIME,
				"Given up listening on duff circuit\n");
			ccterm(circuit);
			circuit = -1;
			continue;
		}

		if (attempt < 0) L_WARN_1(L_GENERAL, L_TIME,
			"Call opened OK after %d unlogged failures\n",
			-attempt);
		if (outer > 0) L_WARN_2(L_GENERAL, L_TIME,
			"Call opened OK after %d outer loop%s\n",
				outer, (outer == 1) ? "" : "s");
	}

	if(conn_rc <= 0)
	{	L_WARN_2(L_GENERAL, 0, 
			"Netlisten for %s failed error = %d\n",
				str, contp.c_TSevent); /*failed*/
		ccterm(circuit);
		circuit = -1;
		return(1);
	}
	ccontrol(circuit, CACCEPT, &contp);
	contp.c_TSradr[contp.c_TSralen] = '\0';
	contp.c_TSlname[contp.c_TSlnlen] = '\0';

	L_LOG_2(L_FULL_ADDR, L_DATE | L_TIME, 
		"Netlisten open to %s (%s)\n", remadr, fromdte);
	else L_LOG_0(L_GENERAL, L_DATE | L_TIME, "call received\n");

	stat_addr(remadr);
	/*
	 * try to find out who called us.
	 */
	 stat_state(S_DECODING);
	if(r_addr_trans(remadr, fromdte, hostname, argstring) < 0)
	{	L_WARN_1(L_GENERAL, 0, "can't translate %s\n", remadr);
		unknown_host = 1;
	}
	else	unknown_host = 0;

	nptr->read_buffer = net_read_buffer;
	nptr->write_buffer = net_write_buffer;
	nptr->write_count = BLOCKSIZ;
	net_open=1;
	return(0);
}

/*
 * search address tables and generate host mnemonic
 * from calling address. name is left in 'nhostname'
 */

/*
 * version for Dexpand.
 * Due to the way a Dexpand works you cannot listen on a specific network.
 * so we have to patch it in this code 'A' is JANET 'B' is PSS.
 */

/* If you cannot tell the network from the DEXPAND line, go by the DTE */
#ifndef  UCL
#define	MIXED_LINES
#endif

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
	'p',	"000040000040.pss.",	"",
	'p',	"000000000040.pss.",	"",
	'p',	"2342",			"2342",
	'p',	"gb-",			"234",
/*	'i',	"",			""	THINK ABOUT THIS ONE	*/
};
#define	PREFIXES	((sizeof prefixes) / (sizeof prefixes[0]))
#endif

r_addr_trans(calling, called, nhostname, hn)
char    *calling, *called, *nhostname, *hn;
{
	extern lnetchar;
	char net = lnetchar;
	char dte[ENOUGH];
	int i,len;

	strcpy(dte, calling +1);                      /* +1 for A/B */

	/* Can we get the network from the incoming line, or the DTE? */
#ifdef	MIXED_LINES
#ifndef NOPREFIXHACK
	/* is net set to an existing network letter ? */
	for (i=1; i<PREFIXES; i++) if (net==prefixes[i].netc) break;
	/* No, so set the `default' */
	if (i >= PREFIXES) net=prefixes[0].netc;

	for (i=1; i<PREFIXES; i++) if (strncmpuc(dte, prefixes[i].prefix,
			len=strlen(prefixes[i].prefix)) == 0)
	{	char temp[ENOUGH];
		strcpy(temp, prefixes[i].replace);
		strcat(temp, dte+len);
		strcpy(dte, temp);
		net = prefixes[i].netc; 
	}
#endif
#else	MIXED_LINES
	/*
	 * horrible hack for network to listener fix
	 */
	switch(*calling){
	case 'a':
	case 'A':
	default:                        /* A = JANET */
		net = 'j';
		break;
	case 'b':			/* B = PSS */
	case 'B':
		net = 'p';
		break;
	}

	/*
	 * An IPSS call...
	 */
	if(net == 'p' && strncmp(dte, "2342", 4) != 0)
		net = 'i';
#endif	MIXED_LINES

	L_LOG_4(L_ALWAYS, 0, "TS-Call from <%c.%s=%s|%s>\n",
		net, calling, dte, called);
	return(nrs_reverse(net, called, dte, nhostname));
}
