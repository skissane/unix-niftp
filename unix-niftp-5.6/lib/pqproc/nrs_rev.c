/* unix-niftp lib/pqproc/nrs_rev.c $Revision: 5.5 $ $Date: 90/08/01 13:36:57 $ */
/*
 * $Log:	nrs_rev.c,v $
 * Revision 5.5  90/08/01  13:36:57  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.4  89/08/26  15:40:24  pb
 * Distribution of Aug89PPsupport: Update READMEs for PP
 * 
 * Revision 5.1  88/10/07  17:23:46  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:27:33  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:04:56  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/04/24  19:25:33  pb
 * Use alias rather than name if available.
 * 
 * Revision 5.0  87/03/23  03:49:22  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include <ctype.h>
#include "ftp.h"
#include "nrs.h"

/*
 * search address tables and generate host mnemonic
 * from calling address. name is left in 'hostname'
 */

/*
 * reverse translate the from address via the NRS database
 */

extern  char	lcomplain;	/* complain about short addresses */
extern  char	lreverse;	/* enforce reverse lookup */
extern  char    ltranstype;
extern  char	why_unknown_host[];
extern	char	*xfer_type;	/* Type of xfer (e.g. with NSAP) */

/* TEMP HACK */
struct {
	char	net_c;
	char	*net_s;
} netnames[] = {
	'j',	"JANET",
	'p',	"PSS",
	'i',	"IPSS",
	'l',	"LOCAL"
};
#define	NETNAMES ((sizeof netnames) / (sizeof netnames[0]))

#ifndef	GCONTEXT
#define GCONTEXT 126	/* Gateway context: should be in ../../h/nrs.h */
#endif	GCONTEXT

#ifndef	MAX_DTETONAME
#define MAX_DTETONAME 10	/* max depth: should be in ../../h/nrs.h */
#endif	MAX_DTETONAME

struct host_entry	*nrs_dtetoname();
char			*index();

int transtype2context(transtype)
{	switch(transtype)
	{
	case T_PP:
	case T_MAIL:	return MCONTEXT;
	case T_NEWS:	return NCONTEXT;	
	case T_FTP:	return FCONTEXT;
	case T_JTMP:	return JCONTEXT;
	}
	return -1;
}

nrs_reverse(net, called, calling, canon_hostname)
char    net;
char	*called;
char	*calling;
char    *canon_hostname;
{
	L_WARN_4(L_ALWAYS, 0,
		"nrs_reverse(%c, %s, %s, %s) called with no context\n",
		net, called, calling, canon_hostname);
	return nrs_reverse_c(net, -2, called, calling, canon_hostname);
}

nrs_reverse_c(net, context, called, calling, canon_hostname)
char	net;
char	*called;
char	*calling;
char    *canon_hostname;
{
	char	tbuff[ENOUGH];
	char	*Tbuff;
	register char	*p, *q;
	struct  host_entry      *Hp;
	int	ndigits = 0;
	int	fail2;
	int	fail3 = 0;

	ltranstype = 0;
	if(ch_called(xfer_type ? xfer_type : called)<0)
		return(-1);
	if (context == -2) context = transtype2context(ltranstype);
	/*
	 * Look at dte part
	 */
	q = tbuff;
	sprintf(q, "c%d.", context);
	while (*q) q++;
	Tbuff = q;
	*q++ = net;
	*q++ = '.';
	p = calling;

#ifdef  NOLEADINGZEROS
	/* skip any leading zeros in calling address */
	while(*p == '0') {
		ndigits++;
		p++;
	}
#endif  NOLEADINGZEROS

	while('0' <= *p && *p <= '9') {
		ndigits++;
		*q++ = *p++;
	}

	/* Looks like <nsap>.<MAC or X.121> */
	if (ndigits > 16 && *p == '.' && isdigit(p[1]))
	{	int rc;
		*q = 0;
		*Tbuff = 'o';
		rc = nsap_validate(Tbuff+2, net, p+1);
		L_LOG_4(L_GENERAL, 0, "validate `%s', %d, `%s' gave %d\n",
			Tbuff+2, net, p+1, rc);
		if (!rc) {
			rc = nsap_validate(Tbuff+2, 0, p+1);
			L_LOG_4(L_GENERAL, 0, "validate `%s', %d, `%s' gave %d\n",
				Tbuff+2, 0, p+1, rc);
		}
		if (!rc) fail3 = 1;
	}
	else
	{

#ifdef	CAMTEC_HACK
		/*
		 * HACK for CAMTEC pad omitting YBTS separator ....
		 */
		if ((p != calling) &&
		    (('A' <= *p && *p <= 'Z') || ('a' <= *p && *p <= 'z')))
			*q++ = '.';
#endif	CAMTEC_HACK

		/*
		 * have to fix this.... to cope with bum characters at end of string
		 */
		while(*p)
			if('A' <= *p && *p <= 'Z')
				*q++ = *p++ + 'a' - 'A';
			else
				*q++ = *p++;
	}
	*q = 0;

	fail2 = (lcomplain && ndigits < 12);

	Hp = nrs_dtetoname(tbuff, 0);
	if (!Hp) Hp = nrs_dtetoname(Tbuff, 0);
	dbase_end();
	if(!Hp){                         /* address not found */
		int i;
		char *istr = netnames[0].net_s;	/* default is the first one */

		for (i=0; i<NETNAMES; i++) if (net == netnames[i].net_c)
		{	istr = netnames[i].net_s;
			break;
		}

		sprintf(canon_hostname,"[+%s.%s]", istr, &Tbuff[2]); /* skip q. */
		sprintf(why_unknown_host, "Cannot find %s in the NRS database",
			canon_hostname);
		L_WARN_1(L_ALWAYS, 0, "%s\n", why_unknown_host);
		if(fail2)
			printf("****short DTE address\n");
		return(-1);
	}

	(void) strcpy(canon_hostname, (!uselongform && Hp->host_alias) ?
				Hp->host_alias : Hp->host_name);

	if(lreverse){ /* remainder of enforcement in rsft */
		int oldtype = ltranstype;
		switch(Hp->n_context){
		case FCONTEXT:
			if(!FTPTRANS)
				ltranstype = 0;
			break;
#ifdef	MAIL
		case MCONTEXT:
			if(!MAILSTRANS)
				ltranstype = 0;
			break;
#endif	MAIL
#ifdef JTMP
		case JCONTEXT:
			if(!JTMPTRANS)
				ltranstype = 0;
			break;
#endif JTMP
#ifdef NEWS
		case NCONTEXT:
			if(!NEWSTRANS)
				ltranstype = 0;
			break;
#endif NEWS
		default:
			L_WARN_1(L_GENERAL, 0, "Unexpected context %d\n",
				Hp->n_context);
			ltranstype = 0;
		}
		if(!ltranstype){
			(void) sprintf(why_unknown_host,
				"Illegal called context (%d/%d)",
				oldtype, Hp->n_context);
#ifdef	MAIL
			L_WARN_4(L_GENERAL, 0, "from %s with %s (%x|%x)\n",
				canon_hostname, why_unknown_host,
				MAILSTRANS, FTPTRANS);
#else
			L_WARN_4(L_GENERAL, 0, "from %s with %s (%x|%x)\n",
				canon_hostname, why_unknown_host,
				0x4321, FTPTRANS);
#endif
			return(-1);
		}
	}

	if(fail3)
	{	sprintf(why_unknown_host,
			"Cannot verify NSAP `%s' from `%s'",
			Tbuff+2, p+1);
		L_ACCNT_2(L_ALWAYS, 0, "**** Host:- %s with duff SNPA - %s\n",
			canon_hostname, calling);
		L_WARN_1(L_ALWAYS, 0, "*** %s\n", why_unknown_host);
		if (lreverse) return -1;
	}
	else if(fail2)
	{     L_ACCNT_1(L_ALWAYS, 0, "****Host:- %s with short addr\n", canon_hostname);
	}
	else  L_ACCNT_1(L_ALWAYS, L_TIME, "Host:- %s\n", canon_hostname);
	return(0);
}

struct host_entry *nrs_dtetoname(dteaddr, depth)
char *dteaddr;
{
	register struct host_entry *hp;
	register int i;
	register char *gp;
	char newdte[ENOUGH];

	L_DEBUG_2(L_DEB_ADDR,0,"nrs_dtetoname(%s, %d)\n",dteaddr, depth);

	if (depth > MAX_DTETONAME)
	{	L_DEBUG_2(L_ALWAYS,0,"nrs_dtetoname looping on %s (%d)\n",
			dteaddr, depth);
		return (struct host_entry *)0;
	}

	/* Easy case -- an explicit entry */
	if(hp = dbase_get(dteaddr)) return hp;

	L_DEBUG_2(L_DEB_ADDR,0,"%s gave %x\n", dteaddr, hp);
	/* Hmmm ... we have [c<c>.]<n>.<DTE>/<rest> -- try <n>.<DTE>.<rest> */
	if(gp = index(dteaddr,'/'))
	{	*gp = '.';
		if(hp = dbase_get(dteaddr)) return hp;
		*gp = '/';
	}

	/* Maybe it is [c<c>.]<n>.<gatewayinfo><sep><DTE><sep><rest> */
	/* Look for an entry such as:
	 * h:uk.ac.pss-gate:uk.ac.pss-gate:janet:126:p.#::000000000040:::::
	 * Which means that "janet" + "000000000040" + sep + rest
	 * -> "p." + rest
	 */
	for (gp = dteaddr + strlen(dteaddr)-1; gp > dteaddr; gp--)
	{	char was = *gp;

		/* Is this a possible separator character */
		switch (was)
		{
		case '.':
		case '/':
		case '+':	break;

/*		default:	if ?? continue; */
		}

		*gp = 0;
		L_DEBUG_1(L_DEB_ADDR,0,"dbase_get(%s)\n",dteaddr);
		hp = dbase_get(dteaddr);
		*gp = was;
		if(hp) for(i = 0 ; i < hp->n_nets ; i++)
		{	int len;
			struct NETWORK *np;
			char *newdata;
			char *restofaddress;
			net_entry *netp = &hp->n_addrs[i];
			struct host_entry *next;

			L_DEBUG_3(L_DEB_ADDR, 0, "net %d/%x %x\n",
				i, netp, netp->net_name);
			if(!netp->net_name) continue;
			for(np = NETWORKS ; np->Nname ; np++)
				if(!same_net(netp->net_name, np->Nname, 1))
					break;
			L_DEBUG_2(L_DEB_ADDR, 0, "Net %x, ga %x\n",
				np->Nname, netp->gate_addr);
			/* an unknown network on this host */
			if(!np->Nname) continue;
			if(!netp->gate_addr) break;
			/* We've found a gateway into another network.
			 * Try to find a network with the remaining
			 * address as a valid DTE. The gp string
			 * starts with a '.' (!!)
			 */
			L_DEBUG_2(L_DEB_ADDR, 0, "cp %x to %x\n",
				netp->gate_addr, newdte);
			newdata = netp->gate_addr;
			if (!strncmp(newdata, "DT ", 3)) newdata += 3;
			strcpy(newdte, newdata);
			restofaddress = gp;
			if (newdte[len=(strlen(newdte) -1)] == '#')
				restofaddress++, newdte[len]='\0';
			strcat(newdte, restofaddress);
			L_DEBUG_3(L_GENERAL,0,"Gateway %s (%s) -> %s\n",
				hp->host_alias, hp->host_name, newdte);
			if(next = nrs_dtetoname(newdte, depth+1))
			{	L_DEBUG_2(L_GENERAL,0,"%s => %s\n",
					newdte,	next->host_name);
				return next;
			}
			L_DEBUG_1(L_GENERAL,0,"%s lookup failed\n", newdte);
		}
	}
	L_DEBUG_0(L_DEB_ADDR,0,"dbase_get(), so give up\n");
	return (struct host_entry *)0;
}

/* Look up the service name for a specified called address */
ch_called(called)
char	*called;
{
	int     i;
	register char   *p, *q, c;

	for(i = 0; p = q_addrtype[i].at_str; i++){
		for(q = called; *q && *p ; q++, p++){
			if( (c = *q) >= 'A' && c <= 'Z')
				c += 'a' - 'A';
			if(*p != c)
				break;
		}
		if(!*q && !*p){
			L_ACCNT_2(L_ALWAYS, 0, "got type %s - %d\n",
				q_addrtype[i].at_str, q_addrtype[i].at_type);
			ltranstype = q_addrtype[i].at_type;
			return(0);
		}
	}
	(void) sprintf(why_unknown_host, "Unrecognisable called string '%s'",
								called);
	L_WARN_1(L_ALWAYS, 0, "**** %s\n", why_unknown_host);
	return(-1);
}
