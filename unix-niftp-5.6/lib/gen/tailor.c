#ifndef	lint		/* unix-niftp lib/gen/tailor.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/lib/gen/tailor.c,v 5.6.1.6 1993/01/10 07:10:24 pb Rel $";
#endif	lint

/*
 * Read the tailor file
 *  Format of configure file is:-
 *   ITEM name [value][,value]*
 *  or
 *   ITEM value
 *
 * $Log: tailor.c,v $
 * Revision 5.6.1.6  1993/01/10  07:10:24  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:01:09  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:35:33  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.3  89/07/16  12:03:10  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:46:37  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:14:53  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  07:12:28  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.4  88/01/28  06:11:34  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:30:48  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:26:40  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:04:05  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/06/11  11:42:23  pb
 * Add network tailoring.
 * 
 * Revision 5.0  87/03/23  03:36:08  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "ftp.h"
#include "nrs.h"
#include "hcontrol.h"

#define	T_NULL    	0
#define	T_ALIAS		1
#define	T_ALTPWFILE	43
#define	T_BANNEDF	3
#define	T_BANNEDU	2
#define	T_BINDIR	4
#define	T_DISKFULL	5
#define	T_DOCKETDIR	6
#define	T_DOMAIN	7
#define	T_DQUEUE	8
#define	T_FTPUSER	9
#define	T_GUESTUSER	44
#define	T_HOST		10
#define	T_KEYFILE	11
#define	T_KILLSPOOL	12
#define	T_LISTEN	13
#define	T_LOGDIR	14
#define	T_LONGFORM	15
#define	T_MAILDIR	16
#define	T_MAILFMT	17
#define	T_MAILPROG	18
#define	T_NET		19
#define	T_OURNAME	20
#define	T_OUTDTYPE	21
#define	T_PRINTER	22
#define	T_QUEUE		23
#define	T_SECURE	24
#define	T_SETUP		25
#define	T_SPOOLER	26
#define	T_TABLE		27
/* these should be tailored on a per queue basis */
#define	T_PADDRTYPE	28
#define	T_QADDRTYPE	29
#define	T_QRETRIES	30
#define	T_QTIMEOUT	31

#ifdef JTMP
#define	T_JTMPDIR	32
#define	T_JTMPPROC	33
#define	T_JTMPUSER	34
#endif JTMP
#ifdef MAIL
#define	T_MAILUSER	35
#endif MAIL
#ifdef	PP
#define	T_PPDIR		36
#define	T_PPPROC	37
#define	T_PPUSER	38
#define	T_PPCHAN	42
#endif	PP
#ifdef NEWS
#define	T_NEWSUSER	39
#define	T_NEWSDIR	40
#define	T_NEWSPROC	41
#endif NEWS

#define	O_LOCAL   1
#define	O_QUEUE   2
#define	O_LEVEL   3
#define	O_ADDRESS 4
#define	O_SHOW    5
#define	O_NFCS	  6
#define	O_FCS	  7
#define	O_PKT	  8
#define	O_WND	  9

#define	B_TRUE    1
#define	B_FALSE   2

#define	L_ADDRESS 1
#define	L_CHANNEL 2
#define	L_LEVEL   3
#define	L_PROG    4
#define	L_LOGFILE 5
#define	L_INFOMSG 6
#define	L_OPTS    7
#define	L_NCHAR   8
#define	L_STATFILE 9
#define	Q_DIR     10
#define	Q_DBASE   11
#define	Q_MASTER  12
#define	L_ERRFILE 13
#define	Q_ORDERED 14
#define	Q_BACKOFF 15

#define	D_PERCENT	1
#define	D_BYTES		2
#define	D_TYPE		3

#define	L_LREJECT   1
#define	L_LCOMPLAIN 2
#define	L_LCREJECT  3
#define	L_LREVERSE  4
#define	L_LALLOW_TJI 5

/* structure to hold names of all tokens */
struct  nvals   {
	char    *tval;
	int     toke;
};

static  struct  nvals   items[] = {
	"ALIAS",	T_ALIAS,
	"ALTPWFILE",	T_ALTPWFILE,
	"BANNEDUSERS",	T_BANNEDU,
	"BANNEDFILE",	T_BANNEDF,
	"BINDIR",	T_BINDIR,
	"DISKFULL",	T_DISKFULL,
	"DISCFULL",	T_DISKFULL,
	"DOCKETDIR",	T_DOCKETDIR,
	"DOMAIN",	T_DOMAIN,
	"DQUEUE",	T_DQUEUE,
	"FTPUSER",	T_FTPUSER,
	"GUESTUSER",	T_GUESTUSER,
	"HOST",		T_HOST,
#ifdef JTMP
	"JMTPDIR",	T_JTMPDIR,
	"JTMPPROC",	T_JTMPPROC,
	"JTMPUSER",	T_JTMPUSER,
#endif JTMP
	"KEYFILE",	T_KEYFILE,
	"KILLSPOOL",	T_KILLSPOOL,
	"LISTEN",	T_LISTEN,
	"LOGDIR",	T_LOGDIR,
	"LONGFORM",	T_LONGFORM,
	"MAILDIR",	T_MAILDIR,
	"MAILPROG",	T_MAILPROG,
	"MAILFMT",	T_MAILFMT,
#ifdef MAIL
	"MAILUSER",	T_MAILUSER,
#endif MAIL
	"NET",		T_NET,
#ifdef NEWS
	"NEWSDIR",	T_NEWSDIR,
	"NEWSPROC",	T_NEWSPROC,
	"NEWSUSER",	T_NEWSUSER,
#endif NEWS
	"OUTDTYPE",	T_OUTDTYPE,
	"OURNAME",	T_OURNAME,
	"PADDRTYPE",	T_PADDRTYPE,
#ifdef PP
	"PPDIR",	T_PPDIR,
	"PPCHAN",	T_PPCHAN,
	"PPPROC",	T_PPPROC,
	"PPUSER",	T_PPUSER,
#endif PP
	"PRINTER",	T_PRINTER,
	"QADDRTYPE",	T_QADDRTYPE,
	"QRETRIES",	T_QRETRIES,
	"QTIMEOUT",	T_QTIMEOUT,
	"QUEUE",	T_QUEUE,
	"SECUREDIRS",	T_SECURE,
	"SETUPPROG",	T_SETUP,
	"SPOOLER",	T_SPOOLER,
	"TABLE",	T_TABLE,
	0, 0,
};

static  struct  nvals   options[] = {
	"local",   O_LOCAL,
	"queue",   O_QUEUE,
	"level",   O_LEVEL,
	"address", O_ADDRESS,
	"show",    O_SHOW,
	"nfcs",    O_NFCS,
	"fcs",     O_FCS,
	"pkt",     O_PKT,
	"wnd",     O_WND,
	0, 0,
};

static  struct  nvals   bools[] = {
	"true",    B_TRUE,
	"false",   B_FALSE,
	0, 0,
};

static  struct  nvals   dopts[] = {
	"percent",  D_PERCENT,
	"bytes",    D_BYTES,
	"type",     D_TYPE,
	0, 0,
};

static  struct  nvals   lopts[] = {
	"address",  L_ADDRESS,
	"channel",  L_CHANNEL,
	"level",    L_LEVEL,
	"prog",     L_PROG,
	"logfile",  L_LOGFILE,
	"errfile",  L_ERRFILE,
	"statfile", L_STATFILE,
	"infomsg",  L_INFOMSG,
	"opts",     L_OPTS,
	"nchar",    L_NCHAR,
	"dir",      Q_DIR,
	"dbase",    Q_DBASE,
	"master",   Q_MASTER,
	"ordered",  Q_ORDERED,
	"backoff",  Q_BACKOFF,
	0, 0,
};

static  struct  nvals   lflags[] = {
	"reject",  L_LREJECT,
	"complain",L_LCOMPLAIN,
	"creject", L_LCREJECT,
	"reverse", L_LREVERSE,
	"allow_tji",L_LALLOW_TJI,
	0, 0,
};

static  struct  nvals   addrtypes[] = {
	"ftp",		T_FTP,
	"niftp",	T_FTP,
	"mail",		T_MAIL,
	"niftp-mail",	T_MAIL,
	"niftp.mail",	T_MAIL,
	"jtmp",		T_JTMP,
	"news",		T_NEWS,
	"pp",		T_PP,
	"niftp.pp",	T_PP,
	0, 0,
};

static  char    nrsdat[BUFSIZ*4];         /* place to store strings */
static  char    *nrsp;
static  N_naliases;
static  N_ndiskfulls;
static  N_ndomains;
static  N_nqueues;
static  N_nnets;
static  N_nlistens;
static  N_outdtype;
static  N_qaddrtype;
static  N_paddrtype;
static	N_mailfmt;
static  tget();

static  tascan(), tbscan(), tdscan(), tfscan(), thscan(), tlscan(),
	tnscan(), tpscan(), tqscan(), tuscan(), tzscan();
static	skiptospace(), skipspace();
extern  long atol();

static	line_no;

nrs_init()
{
	register FILE    *NRSP;
	register c;
	char   *p, *q;
	char    lbuf[BUFSIZ];
	char    ibuf[BUFSIZ];
	static  inited;

	if(inited)
		return(0);
	if( (NRSP = fopen(NRSTAILOR, "r")) == NULL)
	{	L_ERROR_1(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Failed to open `%s'", NRSTAILOR);
		return(-1);
	}
	setbuf(NRSP, ibuf);

	nrsp = nrsdat;
	inited = 1;
	line_no = 0;

	for(;;){
		for(p = lbuf ; (c = getc(NRSP)) != EOF && c != '\n' ; *p++ = c)continue;
		if(c == EOF)
			break;
		line_no ++;
		*p = 0;
		if(p == lbuf || *lbuf == '#')   /* blanks / comments */
			continue;
		p = lbuf;
		if(!skipspace(&p, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				 "Line %d: Null keyword `%s'\n",
				 line_no, lbuf);
			 continue;
		}
		q = p;
		if(!skiptospace(&p, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				 "Line %d: Single keyword `%s'\n",
				 line_no, lbuf);
			 continue;
		}
		*p++ = 0;
		skipspace(&p, 0);
		/*
		 * got ITEM pointed to by q
		 */

		switch(tget(q, items)){
		case T_ALIAS:	tascan(p, NETALIASES, &N_naliases);break;
		case T_ALTPWFILE:tbscan(p, &altpwfile);		break;
		case T_BINDIR:	tbscan(p, &BINDIR);		break;
		case T_BANNEDU:	tbscan(p, &banned_users);	break;
		case T_BANNEDF:	tbscan(p, &banned_file);	break;
		case T_DISKFULL:tzscan(p);			break;
		case T_DOCKETDIR:tbscan(p, &DOCKETDIR);		break;
		case T_DOMAIN:	tdscan(p, NRSdomains, &N_ndomains, 1);	break;
		case T_DQUEUE:	tbscan(p, &NRSdqueue);		break;
		case T_FTPUSER:	tuscan(p, &FTPuser, &FTPuid);	break;
		case T_GUESTUSER:tbscan(p, &guest_name);	break;
					/* an as yet unimplemented hook */
		case T_HOST:	thscan(p);			break;
#ifdef JTMP
		case T_JTMPDIR:	tbscan(p, &JTMPdir);		break;
		case T_JTMPPROC:tbscan(p, &JTMPproc);		break;
		case T_JTMPUSER:tuscan(p, &JTMPuser, &JMTPuid);	break;
#endif JTMP
		case T_KEYFILE:	tbscan(p, &KEYFILE);		break;
		case T_KILLSPOOL:tbscan(p, &KILLSPOOL);		break;
		case T_LISTEN:	tlscan(p);			break;
		case T_LOGDIR:	tbscan(p, &LOGDIR);		break;
		case T_LONGFORM:tcscan(p, &uselongform);	break;
		case T_MAILDIR:	tbscan(p, &MAILDIR);		break;
		case T_MAILPROG:tbscan(p, &mailprog);		break;
		case T_MAILFMT:	tfscan(p, mailfmt, &N_mailfmt);	break;
#ifdef MAIL
		case T_MAILUSER:tuscan(p, &MAILuser, &MAILuid);	break;
#endif MAIL
		case T_NET:	tnscan(p);			break;
#ifdef NEWS
		case T_NEWSUSER:tuscan(p, &NEWSuser, &NEWSuid);	break;
		case T_NEWSDIR:	tbscan(p, &NEWSdir);		break;
		case T_NEWSPROC:tbscan(p, &NEWSproc);		break;
#endif NEWS
		case T_OURNAME:	tbscan(p, &ourname);		break;
		case T_OUTDTYPE:tdscan(p, outdtype, &N_outdtype, 0);	break;
		case T_PADDRTYPE:tpscan(p, p_addrtype, &N_paddrtype);	break;
#ifdef PP
		case T_PPUSER:	tuscan(p, &PPuser, &PPuid);	break;
		case T_PPDIR:	tbscan(p, &PPdir);		break;
		case T_PPCHAN:	tbscan(p, &PPchan);		break;
		case T_PPPROC:	tbscan(p, &PPproc);		break;
#endif PP
		case T_PRINTER:	tbscan(p, &printer);		break;
		case T_QUEUE:	tqscan(p);			break;
		case T_QADDRTYPE:tpscan(p, q_addrtype, &N_qaddrtype);	break;
		case T_SECURE:	tbscan(p, &SECUREDIRS);		break;
		case T_SETUP:	tbscan(p, &SETUPPROG);		break;
		case T_SPOOLER:	tbscan(p, &NRSdspooler);	break;
		case T_TABLE:	tbscan(p, &NRSdbase);		break;
		case T_QRETRIES:
			if ((Qretries = atoi(p)) == 0) /* SANITY */
				Qretries = QRETRIES;
			break;
		case T_QTIMEOUT:
			if ((Qtimeout = atol(p)*60*60) == 0) /* SANITY */
				Qtimeout = QTIMEOUT;
			break;
		default:
			L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				 "Line %d: Unrecognised keyword `%s'\n",
				 line_no, q); continue;
		}
	}
	if(N_naliases)		NETALIASES[N_naliases].NA_alias	= 0;
	if(N_nqueues)		NRSqueues[N_nqueues]	= 0;
	if(N_ndomains)		NRSdomains[N_ndomains]	= 0;
	if(N_ndiskfulls)	DISKFULLS[N_ndiskfulls].Ddevice	= 0;
	if(N_nnets)		NRSnetworks[N_nnets]	= 0;
	if(N_nlistens)		LISTENS[N_nlistens].Lname=0;
	if(N_outdtype)		outdtype[N_outdtype]	= 0;
	if(N_paddrtype)		p_addrtype[N_paddrtype].at_str	= 0;
	if(N_qaddrtype)		q_addrtype[N_qaddrtype].at_str	= 0;
	if(N_mailfmt)		mailfmt[N_mailfmt].prog	= 0;
#ifdef  pdp11
	setbuf(NRSP, NULL);       /* FIX BUG IN PDP11's */
#endif
	fclose(NRSP);
	return(0);
}


static skipspace(pp, comma)
char **pp;
{	while(**pp == ' ' || **pp == '\t' || (comma && **pp == ',')) (*pp)++;
	return **pp;
}

static skiptospace(pp, comma)
char **pp;
{	while(**pp && (**pp != ' ' && **pp != '\t' && (!comma || **pp != ',')))
		(*pp)++;
	return **pp;
}

/* Copy data from **p to **q */
static copy_str(pp, qq, equal)
char **pp;
char **qq;
{	char *p = *pp;
	char *q = *qq;
	int rc = 1;

	if (!p | !q | !*p)	return 0;

	if(*p == '"')
	{	for(p++; *p && *p != '"' ; *q++ = *p++)
			if (*p == '\\' && p[1]) p++;
		if (*p == '"')	p++;
		else		rc = 0;
	}
	else	while(*p && *p != ' ' && *p != '\t' && *p != ',' &&
			(!equal || *p != '=')) *q++ = *p++;

	if (rc)
	{	skipspace(&p, 1);
		*q++ = '\0';
	}

	*pp = p;
	*qq = q;

	return rc;
}

/*
 * assumes all keys in the table are letters
 */
static
tget(string, table)
char    *string;
register struct nvals   *table;
{
	register char   *p, *q;

	for(; (q = table->tval) != NULL;table++)
		for(p =string; *p == *q || ((*p|040) == (*q|040)) ; p++, q++)
			if(!*p)
				return(table->toke);
	return(T_NULL);
}

/* scan the relevent bits */

/* the table entry */
static
tbscan(p, tb)
char   *p;
char    **tb;
{
	char   *q = nrsp;
	if (copy_str(&p, &q, 0))	{ *tb = nrsp;	nrsp = q;}
	else	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			 "Line %d: Null table entry `%s'\n",
			 line_no, q);
}

/* Find aliases */
static
tascan(p, tb, count)
char   *p;
struct NETALIAS *tb;
int	*count;
{
	char   *q;
	int	ocount = *count;

	for(;; (*count)++){
		q = nrsp;

		if (! *p && (ocount != *count)) break;

		/* first get the alias */
		if (!copy_str(&p, &q, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			 "Line %d: Null Alias name `%s'\n",
			 line_no, q);
		 	break;
		}
		tb[*count].NA_alias = nrsp;
		nrsp = q;

		if(*p)	copy_str(&p, &q, 0);
		else
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			 "Line %d: Null alias value for `%s'\n",
			 line_no, tb[*count].NA_alias);
		}

		tb[*count].NA_realname = nrsp;
		nrsp = q;
	}
}

/* MAILFMT	"<local notification>" "<remote-mail delivery>" flags */
static 
tfscan(p, tb, pn)
char *p;
struct mailfmt *tb;
int *pn;
{
	char *q = nrsp;
	int	 flags;
	char	*form;
	char	*prog;
	char	*deliver;

	/* Read the programme name */
	prog = q;
	if (!copy_str(&p, &q, 0))
	{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null programme name `%s'\n",
			 line_no, prog);
		return;
	}

	/* Read the format string for local notification */
	form = q;
	if (!copy_str(&p, &q, 0))
	{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null format `%s'\n",
			 line_no, prog);
		return;
	}

	/* Read the format string for delivery of incoming FTP.MAIL */
	deliver = q;
	if (!copy_str(&p, &q, 0))
	{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null format `%s'\n",
			 line_no, prog);
		return;
	}

	/* Read the flags (just a number at the moment ...) */
	flags = atoi(p);
	if (!flags && *p != '0')
	{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null flags `%s'\n",
			 line_no, prog);
		return;
	}

	if (*pn >= CONF_MAX)
	{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: too many programmes -- `%s'\n",
			 line_no, prog);
		return;
	}

	/* it's all OK */
	nrsp = q;
	tb[*pn].prog	= prog;
	tb[*pn].form	= form;
	tb[*pn].deliver	= deliver;
	tb[*pn].flags	= flags;
	pn++;
}

static
tnscan(p)
char   *p;
{
	char   *q = nrsp;
	char   *xp;
	register struct NETWORK *np;
	char    lbuf[BUFSIZ];

	/*
	 * first get the name of the network
	 */
	if (!copy_str(&p, &q, 0))
	{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: null network -- `%s'\n",
			 line_no, nrsp);
		return;
	}
	NRSnetworks[N_nnets++] = nrsp;
	for(np = NETWORKS ; np->Nname ; np++)
		if(strcmp(np->Nname, nrsp) == 0)
			break;
	if(np->Nname == NULL)
	{	bzero(np, sizeof *np);

		np->Nname = nrsp;
	}
	nrsp = q;

	for(;;){
		xp = lbuf;
		if (!copy_str(&p, &xp, 1))	break;
		if (*lbuf == '#')		break;
		if(*p++ != '='){
			while(*p && *p != ',')
				p++;
			if(!*p)
			{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: null value for `%s'\n",
			 	line_no, lbuf);
				break;
			}
			continue;
		}
		if (!copy_str(&p, &q, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null value for `%s'\n",
		 	line_no, lbuf);
			break;
		}

		switch(tget(lbuf, options)){
		case O_LOCAL:
			switch(tget(nrsp, bools)){
			case B_TRUE:    /* zap the network */
				np->Nopts |= N_LOCAL;
				break;
			case B_FALSE:
				np->Nopts &= ~N_LOCAL;
				break;
			default:
				L_ERROR_1(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				 "Line %d: `true' or `false' expected after local\n",
				 line_no);
			}
			q = nrsp;
			continue;
		case O_QUEUE:
			np->Nqueue = nrsp;
			break;
		case O_LEVEL:
			np->Nloglevel = atoi(nrsp);
			q = nrsp;
			continue;
		case O_ADDRESS:
			np->Naddr = nrsp;
			break;
		case O_SHOW:
			np->Nshow = nrsp;
			break;
		case O_NFCS:
			switch(tget(nrsp, bools)){
			case B_TRUE:    /* zap the network */
				np->Nopts |= N_NFCS;
				break;
			case B_FALSE:
				np->Nopts &= ~N_NFCS;
				break;
			default:
				L_ERROR_1(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				 "Line %d: `true' or `false' expected after nfcs\n", line_no);
			}
			q = nrsp;
			continue;
		case O_FCS:
			switch(tget(nrsp, bools)){
			case B_TRUE:    /* zap the network */
				np->Nopts |= N_FCS;
				break;
			case B_FALSE:
				np->Nopts &= ~N_FCS;
				break;
			default:
				L_ERROR_1(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				 "Line %d: `true' or `false' expected after fcs\n", line_no);
			}
			q = nrsp;
			continue;
		case O_PKT:
			np->Npkt_size = atoi(nrsp);
			q = nrsp;
			continue;
		case O_WND:
			np->Nwnd_size = atoi(nrsp);
			q = nrsp;
			continue;
		default:
			L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				 "Line %d: unrecognided key `%s'\n", line_no, lbuf);
			q = nrsp;
			break;
		}
		nrsp = q;
	}
}

static
tuscan(p, tb, pint)
char *p;
char **tb;
int  *pint;
{	struct passwd *pw;

	tbscan(p, tb);
	if (pint && (pw = getpwnam(*tb))) *pint = pw->pw_uid;
	else L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
		"Line %d: uid not set to `%s'\n", line_no, *tb);
}

static
tcscan(p, pbool)
char *p;
int *pbool;
{	
	char *s;
	tbscan(p, &s);
	switch(tget(s, bools)){
	case B_TRUE:    *pbool = 1; break;
	case B_FALSE:	*pbool = 0; break;
	default:
			L_ERROR_1(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: `true' or `false' expected after longform\n", line_no);
	}
	nrsp = s;	/* We don't want to keep it */
}

static
tdscan(p, tb, count, dot)
char   *p;
char    **tb;
int     *count;
{
	char   *q = nrsp;

	for(;;){
		if(!skipspace(&p, 0))	break;
		if (!copy_str(&p, &q, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: null domain `%s'\n", line_no, q);
			break;
		}
		if(dot && (*nrsp != '.') && (q[-2] != '.'))
		{ q[-1] = '.';	*q++ = 0; }
		tb[(*count)++] = nrsp;
		nrsp = q;
	}
}

/* fill in the listeners information */

static
tlscan(p)
char   *p;
{
	register struct LISTEN  *np;
	char   *q = nrsp;
	char   *xp;
	char    lbuf[BUFSIZ];

	/*
	 * first get the name of the network
	 */
	if (!copy_str(&p, &q, 0))
	{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null listener name `%s'\n", line_no, q);
		return;
	}
	for(np = LISTENS ; np->Lname ; np++)
		if(strcmp(np->Lname, nrsp) == 0)
			break;
	if(np->Lname == NULL){
		np->Lname = nrsp;
		nrsp = q;
		N_nlistens++;
	}
	else
		q = nrsp;

	for(;;){
		xp = lbuf;
		if (!copy_str(&p, &xp, 1))	break;
		if (*lbuf == '#')		break;
		if(*p++ != '='){
			while(*p && *p != ',')	p++;
			if(!*p)
			{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: null value for `%s'\n",
			 	line_no, lbuf);
				break;
			}
			continue;
		}
		if (!copy_str(&p, &q, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null value for `%s'\n",
		 	line_no, lbuf);
			break;
		}
		switch(tget(lbuf, lopts)){
		case L_ADDRESS:
			np->Laddress = nrsp;
			break;
		case L_CHANNEL:
			np->Lchannel = nrsp;
			break;
		case L_LEVEL:
			np->Llevel = atoi(nrsp);
			q = nrsp;
			break;
		case L_PROG:
			np->Lprog = nrsp;
			break;
		case L_STATFILE:
			np->Lstatfile = nrsp;
			break;
		case L_LOGFILE:
			np->Llogfile = nrsp;
			break;
		case L_ERRFILE:
			np->Lerrfile = nrsp;
			break;
		case L_INFOMSG:
			np->Linfomsg = nrsp;
			break;
		case L_NCHAR:
			np->Lnchar = *nrsp;
			q = nrsp;
			continue;
		case L_OPTS:
			switch(tget(nrsp, lflags)){
			case L_LREJECT:         /* always reject */
				np->Lopts |= L_REJECT;
				break;
			case L_LCOMPLAIN:       /*complain about short addrs*/
				np->Lopts |= L_COMPLAIN;
				break;
			case L_LCREJECT:        /* reject duff calling addrs*/
				np->Lopts |= L_CREJECT; /*should be default */
				break;
			case L_LREVERSE: /* reject on failed reverse lookup */
				np->Lopts |= L_REVERSE;
				break;
			case L_LALLOW_TJI: /* Allow Take Job Input */
				np->Lopts |= L_ALLOW_TJI;
				break;
			default:
				L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Invalid local opt `%s'\n", line_no, nrsp);
			}
			q = nrsp;
			continue;
		default:
			L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Unrecognised option `%s'\n", line_no, lbuf);
			q = nrsp;
		}
		nrsp = q;
	}
}


/* fill in the diskfull information */

static
tzscan(p)
char   *p;
{
	char   *q = nrsp;
	char   *xp;
	register struct DISKFULL  *np;
	char    lbuf[BUFSIZ];

	/*
	 * first get the name of the diskfull
	 */
	if (!copy_str(&p, &q, 0))	return;		/* a null string */
	np = &(DISKFULLS[N_ndiskfulls++]);
	np->Ddevice = nrsp;
	nrsp = q;

	for(;;){
		xp = lbuf;
		if (!copy_str(&p, &xp, 1))	break;		/* a null string */
		if (*lbuf == '#')		break;
		if(*p++ != '='){
			while(*p && *p != ',')	p++;
			if(!*p)
			{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: null value for `%s'\n",
			 	line_no, lbuf);
				break;
			}
			continue;
		}
		if (!copy_str(&p, &q, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null value for `%s'\n",
		 	line_no, lbuf);
			break;
		}
		switch(tget(lbuf, dopts)){
		case D_PERCENT:
			np->Dpercent = atoi(nrsp);
			if ((np->Dpercent == 0) && strcmp(nrsp, "0"))
				L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Bad value for DISKFULL % `%s'\n",
				line_no, nrsp);
			q = nrsp;
			break;
		case D_TYPE:
			np->Dtype = tget(nrsp, addrtypes);
			if (np->Dtype == T_NULL)
			{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Bad DISKFULL type `%s'\n",
				line_no, nrsp);
				np->Dtype = T_FTP;
			}
			q = nrsp;
			break;
		case D_BYTES:
			np->Dbytes = atoi(nrsp);
			if ((np->Dbytes == 0) && strcmp(nrsp, "0"))
				L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Bad value for DISKFULL bytes `%s'\n",
				line_no, nrsp);
			q = nrsp;
			break;
		default:
			L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Unrecognised option `%s'\n", line_no, lbuf);
			q = nrsp;
		}
		nrsp = q;
	}
}

/* fill in the YBTS address information */

static
tpscan(p, tb, count)
char   *p;
struct addrtype *tb;
int	*count;
{
	char   *q = nrsp;

	for(;; q=nrsp, (*count)++){
		int type = T_NULL;

		/* first get YBTS string */
		if (!copy_str(&p, &q, 0))	break;
		tb[*count].at_str = nrsp;
		nrsp = q;

		if (copy_str(&p, &q, 0)) type = tget(nrsp, addrtypes);
		else	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: YBTS type missing `%s'\n",
				line_no, tb[*count].at_str);

		if (type == T_NULL)
		{	type = atoi(nrsp);
			if (type == 0)
			{	type = T_FTP;	/* Complain ?? */
				L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Unrecognised YBTS type `%s'\n", line_no, nrsp);
			}
		}
		tb[*count].at_type = type;
	}
}

/* a queue entry item */

static
tqscan(p)
char   *p;
{
	char   *q = nrsp;
	char   *xp;
	register struct QUEUE   *np;
	char    lbuf[BUFSIZ];

	/*
	 * first get the name of the network
	 */
	if(!copy_str(&p, &q, 0))	return;		/* a null string */
	for(np = QUEUES; np->Qname ; np++)
		if(strcmp(np->Qname, nrsp) == 0)
			break;
	if(np->Qname == NULL){
		np->Qname = nrsp;
		NRSqueues[N_nqueues++] = nrsp;
		nrsp = q;
	}
	else
		q = nrsp;

	for(;;){
		xp = lbuf;
		if(!copy_str(&p, &xp, 1))	break;
		if (*lbuf == '#')		break;
		if(*p++ != '='){
			while(*p && *p != ',')	p++;
			if(!*p)
			{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: null value for `%s'\n",
			 	line_no, lbuf);
				break;
			}
			continue;
		}
		if (!copy_str(&p, &q, 0))
		{	L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
			"Line %d: Null value for `%s'\n",
		 	line_no, lbuf);
			break;
		}
		switch(tget(lbuf, lopts)){
		case L_LEVEL:
			np->Qlevel = atoi(nrsp);
			q = nrsp;
			break;
		case L_PROG:
			np->Qprog = nrsp;
			break;
		case L_STATFILE:
			np->Qstatfile = nrsp;
			break;
		case L_LOGFILE:
			np->Qlogfile = nrsp;
			break;
		case L_ERRFILE:
			np->Qerrfile = nrsp;
			break;
		case Q_DIR:
			np->Qdir = nrsp;
			break;
		case Q_DBASE:
			np->Qdbase = nrsp;
			break;
		case Q_MASTER:
			np->Qmaster = nrsp;
			if (! np->Qdir) np->Qdir = nrsp;
			break;
		case Q_ORDERED:
			np->Qordered = atoi(nrsp);
			q = nrsp;
			break;
		case Q_BACKOFF:
			np->Qbackoff = atoi(nrsp);
			{	char slash = index(nrsp, '/');
				if (slash) np->Qbackmax = atoi(slash+1);
				else np->Qbackmax = np->Qbackoff;
			}
			q = nrsp;
			break;
		default:
			L_ERROR_2(L_ALWAYS, L_FILE | L_TIME | L_DATE,
				"Line %d: Unrecognised Q key `%s'\n", line_no, lbuf);
			q = nrsp;
		}
		nrsp = q;
	}
}

/* A Host entry. Unimplemented as yet */

/* ARGSUSED */
static
thscan(p)
char    *p;
{
}
