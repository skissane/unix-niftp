/* uk.ac.nott.cs lib/pqproc/adr_trans.c $Revision: 5.6.1.2 $ $Date: 1992/10/17 05:38:25 $ */
/*
 * $Log: adr_trans.c,v $
 * Revision 5.6.1.2  1992/10/17  05:38:25  pb
 * add code to allow ALIASes to passed in as the channel name (for PP tailoring)
 *
 * Revision 5.6.1.1  1992/10/17  05:36:01  pb
 * start adding NSAP code ...
 *
 * Revision 5.6  1991/06/07  17:01:36  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:36:38  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:23:41  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:58:44  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:53:33  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/03/23  03:49:00  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "ftp.h"
#include "nrs.h"

int	Nopts	= 0;
int	Npkt_size = 0;
int	Nwnd_size = 0;

static char * pbencode();
extern char *hide_pss_pw ();

adr_trans(from, net, top)
char    *from, *net, **top;
{
	int     i;
	int	pb_cntxt;
	register char   *p;
	char    *channel, *dte, *ts;
	char    *sp, *exp = (char *) 0, *nexp = (char *) 0, *fmt;
	struct   host_entry     *hp;
	register net_entry      *np;
	register net_entry      *bestnp;
	int bestval = 3;
	register struct NETWORK *Np;
	struct NETWORK *best = (struct NETWORK *) 0;
	char    *to = *top;
	struct addrtype *ap;
	char	*index();

	if(!tab.l_network){
		L_WARN_0(L_GENERAL, 0, "Got a null channel\n");
		return(1);
	}
	else	channel = (char *)&tab + tab.l_network;
	L_LOG_1(L_10, 0, "channel is %s\n", channel);

	/*
	 * channel is now janet/pss or ether or a network...
	 */
	if( (hp = dbase_get(from)) == NULL){
		L_WARN_1(L_GENERAL, 0, "Cannot find %s in database\n", from);
		return(1);
	}
	for( i = 0, np=hp->n_addrs ; i < hp->n_nets ; i++, np++){
		int alias;
		int calias;
		if(np->net_name == NULL)
			continue;
		/* Any info for this host ? */
		if((alias = same_net(channel, np->net_name, 1)) != NULL)
		{	for(Np = NETWORKS ; Np->Nname ; Np++)
				if(calias = same_net(channel, Np->Nname, 1))
					if (alias == 1)
						goto got;
					else if (calias < bestval) {
						best = Np;
						bestnp = np;
						bestval = calias;
					}
		}
	}

	if (best) { Np = best; np = bestnp; }
	else
	{	L_WARN_1(L_GENERAL, 0, "No good net for %s\n", from);
		return(1);
	}
got:;
	Nopts = Np->Nopts;
	Npkt_size = Np->Npkt_size;
	Nwnd_size = Np->Nwnd_size;
	if((fmt = Np->Naddr) == NULL){
		fmt = "%D%T";
		L_WARN_1(L_GENERAL, 0, 
			"No address format - using default %s\n", fmt);
	}
	L_WARN_4(L_DEB_ADDR, 0, "format <%s>, opts %d, pkt=%d, wnd=%d\n",
		fmt, Nopts, Npkt_size, Nwnd_size);
	for (ap = p_addrtype; ap->at_str; ap++)
		if (ap->at_type == (tab.t_flags & T_TYPE)) {
			int alpha = 0;
			char *p;
			/* Is this text for YBTS, or NSAP ? */
			for (p = ap->at_str; *p; p++) if (!isdigit(*p))
			{	alpha++;
				break;
			}
			L_WARN_2(L_DEB_ADDR, 0, "PDDR `%s' gave alpha=%d\n",
				ap->at_str, alpha);
			if (alpha) exp = ap->at_str;
			else nexp = ap->at_str;
		}
	if (!exp) exp = nexp;
	L_WARN_2(L_DEB_ADDR, 0, "Calling addr info `%s' and `%s'\n", exp, nexp);

	if (exp == (char *)0) {
		L_WARN_1(L_ALWAYS, 0, 
			"p_addrtype has no extension for type (%d)\n",
			tab.t_flags & T_TYPE);
		return(1);
	}

	switch(tab.t_flags & T_TYPE){

#ifdef	PP
	case T_PP:
#endif	/* PP */
	case T_MAIL:
		pb_cntxt = 'M';
		sp = np->mail_addr;
		break;
#ifdef JTMP
	case T_JTMP:
		pb_cntxt = 'J';
		sp = np->jtmp_addr;
		break;
#endif JTMP
#ifdef NEWS
	case T_NEWS:
		pb_cntxt = 'F';
		sp = np->news_addr;
		break;
#endif NEWS
	case T_FTP:
		pb_cntxt = 'F';
		sp = np->ftp_addr;
		break;

	default:
		L_WARN_1(L_GENERAL, 0, "unexpected transfer type (%d)\n",
			tab.t_flags & T_TYPE);
		return(1);
	}

	if (! sp) {
		L_WARN_3(L_GENERAL, 0, "type %d gave %x and %x\n",
			tab.t_flags & T_TYPE, pb_cntxt, sp);
		return(1);
	}
	for(dte = NULL, ts = NULL, p = sp ; p ; p = index(p, '\n') ){
		if(p != sp)
			*p++ = 0;
		if(*p == 'D' && *(p+1) == 'T'){
			/*
			 * got the dte address
			 */
			for(p += 2; *p == ' ' ;p++) continue;
			dte = p;
			continue;
		}
		if(*p == 'Y' && *(p+1) == 'B'){
			/*
			 * got the ts string
			 */
			for(p += 2 ; *p == ' ' ;p++) continue;
			ts = p;
			continue;
		}
	}
	if(dte == NULL || ts == NULL){
		int dtelen = strlen(dte);
		if (dtelen > 15)
			L_WARN_2(L_GENERAL, 0,
				"Got a bad address - DTE=%s, YBTS=%s\n",
				(dte) ? dte : "<Null>",
				(ts) ? ts : "<Null>");
		else {
			L_WARN_2(L_GENERAL, 0,
				"Got a bad address - DTE=%s, YBTS=%s\n",
				(dte) ? dte : "<Null>",
				(ts) ? ts : "<Null>");
			return(1);
		}
	}
	L_DEBUG_3(L_DEB_ADDR, 0, "Fmt is %s, dte is %s, ts is %s\n",
			fmt, dte, (ts) ? hide_pss_pw(ts) : "(null)");
	sp = to;
	for(;*fmt ; fmt++){
		if(*fmt != '%'){
			*to++ = *fmt;
			continue;
		}
		switch(*++fmt){
		default:
			*to++ = *fmt;
			continue;
		case 'D':
			for(p = dte; *p ; *to++ = *p++) continue;
			break;
		case 'T':
			if (ts) for(p = ts; *p ; *to++ = *p++) continue;
			break;
		case 'O':
			for(p = ourname; *p ; *to++ = *p++) continue;
			break;
		case 'P':
			pbencode(from, to, pb_cntxt);
			while(*to) to++;
			break;
		case 'E':
			for(p = exp; *p ; *to++ = *p++) continue;
			break;
		case 'N':
			if (nexp) for(p = nexp; *p ; *to++ = *p++) continue;
			break;
		case 'X':
			*to = 0;
			L_DEBUG_2(L_DEB_ADDR, 0,
				"Completed %s, now process %s\n",
				*top, fmt+1);
			if( (to = *++top) == NULL){
				L_WARN_0(L_GENERAL, 0, "Extra 'X' flag in addr\n");
				return(1);
			}
			break;
		}
	}
	*to = 0;
	if( (p = Np->Nshow) == NULL)
		p = Np->Nname;
	(void) strcpy(net, p);
	L_LOG_1(L_FULL_ADDR, 0, "Translated address is %s\n", sp);
	return(0);
}

#ifndef	PBFORMAT
#define	PBFORMAT "%s"
#endif

static char *
pbencode(data, buffer, context)
char *data;
char *buffer;
{
	char buff[40 +1];
	char *bufp = buff;
	char code;
	if (strlen(data) > (((sizeof buff) / 2) -1))
		return 0;
	*bufp++ = '5';
	*bufp++ = '0';

	switch(context)
	{
	case 'F': case 'X': case 'J': case 'M': code = context;
	default:	code='X';
	}
	code -= 32;
	*bufp++ = (code / 10) + '0';
	*bufp++ = (code % 10) + '0';

	for(; *data; data++)
	{	int n = *data - 32;
		if (n<0 || n > 99)	return 0;
		*bufp++ = (n / 10) + '0';
		*bufp++ = (n % 10) + '0';
	}
	*bufp = '\0';
	sprintf(buffer, PBFORMAT, buff);
	return buffer;
}
