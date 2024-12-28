/* unix-niftp bin/dbencode.c $Revision: 5.6.1.2 $ $Date: 1993/01/10 07:02:09 $ */
/*
 * $Log: dbencode.c,v $
 * Revision 5.6.1.2  1993/01/10  07:02:09  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.5  90/08/01  13:29:46  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:10:04  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:25:38  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:02:07  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:17:25  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "opts.h"
#include "nrs.h"
#include "db.h"

#ifndef ENOUGH
#define ENOUGH  150
#endif

static  char    newDBname[ENOUGH];
int     dberrors;
int	max_errors	= 0;

#include <signal.h>
SIGRET     sigcatch();

char    ibuf[BUFSIZ];

#ifdef  pdp11
typedef int     void;
#endif

char    *savedb;
int     oldstyle;
int 	verbose;

main(argc, argv)
char    **argv;
{
	register char   *p;
	register int c;
	int opt;
	extern int optind;
	extern char *optarg;
	static  int     done;

	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise\n");
		exit(1);
	}
	while ((opt = getopt (argc, argv, "Om:v")) != EOF) {
		switch (opt) {
		case 'm':
			max_errors = atoi(optarg);
			break;
		case 'O':
			oldstyle = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
			fprintf (stderr, "Usage: %s [-Ov] [-m N] [dbase]\n", argv[0]);
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;
	if(argc > 0)                    /* set up the NRSdbase as an arg */
		NRSdbase = argv[0];
	setbuf(stderr, NULL);
	if(signal(SIGHUP, sigcatch) == SIG_IGN)
		(void) signal(SIGHUP, SIG_IGN);
	if(signal(SIGINT, sigcatch) == SIG_IGN)
		(void) signal(SIGINT, SIG_IGN);
	if(signal(SIGQUIT, sigcatch) == SIG_IGN)
		(void) signal(SIGQUIT, SIG_IGN);
	if(signal(SIGTERM, sigcatch) == SIG_IGN)
		(void) signal(SIGTERM, SIG_IGN);
	newdb();
	for(;;done=1){
		if (dberrors > max_errors) break;
		for(p = ibuf; (c = getc(stdin)) != EOF && c != '\n';*p++ = c)
			if (p >= ibuf + sizeof ibuf) {
				fprintf (stderr, "Internal buffer overflow");
				dberrors += max_errors+1;
			}
		if(c == EOF){
			if(done)
				break;
			fprintf(stderr, "Premature EOF\n");
			/* FORCE no new dbm */
			dberrors += max_errors+1;
			break;
		}
		if (verbose) printf ("Add %s\n", ibuf);
		*p = 0;
		switch(*ibuf){
		case '#':
			if(host()  == 0)
				continue;
			break;
		case '$':
			if(reverse() == 0)
				continue;
			break;
		case '>':
			if(alias() == 0)
				continue;
			break;
		case '@':
			if(domain() == 0)
				continue;
			break;
		default:
			fprintf(stderr, "Illegal input string (%02x:%s)\n", *ibuf, ibuf);
			break;
		}
		dberrors++;
	}
	if(dberrors <= max_errors)
	{	if (dberrors) fprintf(stderr, "%d errors ignored\n", max_errors);
		changedb();
	}
	else	fprintf(stderr, "No db created dur to %d error%s\n",
			dberrors, (dberrors == 1) ? "" : "s");
	rexit(1);
}

reverse()
{
	register char   *p = ibuf+1;
	char	*base;
	int     context = 0;

	if (*p > '9') context = *p++ - '0';
	else while ('0' <= *p && *p <= '9') context = context*10 + (*p++)-'0';
	
	if (*p++ != ' ') {
		fprintf(stderr, "Corrupt entry (invald reverse context format)\n");
		return -1;
	}
	base = p;

	for(p = ibuf+3; *p && *p != ' ' ;p++);
	if(*p == 0){
		fprintf(stderr, "Corrupt entry (no address in reverse)\n");
		return(-1);
	}
	*p++ = 0;
	if(dbase_renter(context, p, base)  < 0){
		fprintf(stderr, "Renter failed\n");
		return(-1);
	}
	return(0);
}

domain()
{
	register char   *p;
	domin_entry     dom;

	for(p = ibuf+1; *p && *p != ' ' ; p++);
	if(*p == 0){
		fprintf(stderr, "Corrupt entry\n");
		return(-1);
	}
	*p++ = 0;
	dom.domin_name = ibuf+1;
	dom.domin_host = p;
	if(dbase_denter(&dom) < 0){
		fprintf(stderr, "Denter failed\n");
		return(-1);
	}
	return(0);
}

alias()
{
	register char   *p;
	for(p = ibuf+1; *p && *p != ' ' ; p++);
	if(*p == 0){
		fprintf(stderr, "Corrupt entry\n");
		return(-1);
	}
	*p++ = 0;
	if(dbase_oenter(ibuf+1, p) < 0){
		fprintf(stderr, "Oenter failed\n");
		return(-1);
	}
	return(0);
}

#define SPACE   ' '

host()
{
	struct  host_entry      h;
	register char   *p;
	register net_entry      *np;
	register i;
	long    atol();
	char    *nextc();

	bzero( (char *)&h, sizeof(h));
	h.host_name = ibuf+1;
	h.host_alias = p = nextc(ibuf+1, SPACE);
		/* p now points to the first number */
	p = nextc(p, SPACE);
	h.n_timestamp = atol(p);
	p = nextc(p, SPACE);
	h.h_number = atoi(p);
	p = nextc(p, SPACE);
	h.n_localhost = atoi(p);
	p = nextc(p, SPACE);
	h.n_oldhost = atoi(p);
	p = nextc(p, SPACE);
	h.n_disabled = atoi(p);
	p = nextc(p, SPACE);
	h.n_nets = atoi(p);
	for(i = 0, np = h.n_addrs; i < h.n_nets; i++, np++){
		p = nextc(p, SPACE);
		np->n_ftptrans = atoi(p);
		p = nextc(p, SPACE);
		np->n_ftpgates = atoi(p);
		p = nextc(p, SPACE);
		np->n_mailgates = atoi(p);
		if(!oldstyle){
			p = nextc(p, SPACE);
			np->n_jtmpgates = atoi(p);
			p = nextc(p, SPACE);
			np->n_newsgates = atoi(p);
		}
				/* p now points to the first string */
		np->net_name	= p = nextc(p, SPACE);
		np->ts29_addr	= p = nextc(p, DBSEP);
		np->x29_addr	= p = nextc(p, DBSEP);
		np->ftp_addr	= p = nextc(p, DBSEP);
		np->mail_addr	= p = nextc(p, DBSEP);
		if(!oldstyle) {
			np->jtmp_addr = p = nextc(p, DBSEP);
			np->news_addr = p = nextc(p, DBSEP);
		}
		p = nextc(p, DBSEP);
		if(!*np->net_name)
			np->net_name = NULL;
		if(!*np->ts29_addr)
			np->ts29_addr = NULL;
		else
			expand(np->ts29_addr);
		if(!*np->x29_addr)
			np->x29_addr = NULL;
		else
			expand(np->x29_addr);
		if(!*np->ftp_addr)
			np->ftp_addr = NULL;
		else
			expand(np->ftp_addr);
		if(!*np->mail_addr)
			np->mail_addr = NULL;
		else
			expand(np->mail_addr);
		if(!oldstyle)
			if(!*np->jtmp_addr)
				np->jtmp_addr = NULL;
			else
				expand(np->jtmp_addr);
		else
			np->jtmp_addr = NULL;
		if(!oldstyle)
			if(!*np->news_addr)
				np->news_addr = NULL;
			else
				expand(np->news_addr);
		else
			np->news_addr = NULL;
	}
	if(!oldstyle){
		h.host_info = p;
		p =nextc(p, DBSEP);
		if(!*h.host_info)
			h.host_info = NULL;
		else
			expand(h.host_info);
	}
	if(dbaseenter(&h) < 0){
		fprintf(stderr, "Enter failed\n");
		return(-1);
	}
	return(0);
}

expand(str)
register char   *str;
{
	for(;*str; str++)
		if(*str == '\r')
			*str = '\n';
}

/*
 * go to the next character along and zap
 */

char    *
nextc(ptr, c)
register char   *ptr;
register c;
{
	if(!*ptr){
		fprintf(stderr, "Invalid value in nextc\n");
		dberrors++;
		return(ptr);
	}
	for(;*ptr && *ptr != c ; ptr++);
	if(*ptr)
		*ptr++ = 0;
	return(ptr);
}

/*
 * junk any temporary database
 */

SIGRET sigcatch()
{
#if	SIGRET == void
	rexit(2);
#else
	return rexit(2);
#endif
}

newdb()
{
	char    lbuf1[ENOUGH];
	char    lbuf2[ENOUGH];
	int     fd;

	savedb = NRSdbase;
	(void) strcpy(newDBname, NRSdbase);
	(void) strcat(newDBname, "$");
	sprintf(lbuf1, "%s.dir", newDBname);
	sprintf(lbuf2, "%s.pag", newDBname);
	if( (fd = creat(lbuf1, (0660) | 0444)) < 0){
		fprintf(stderr, "Cannot create %s\n", lbuf1);
		exit(1);
	}
	(void) close(fd);
	if( (fd = creat(lbuf2, (0660) | 0444)) < 0){
		fprintf(stderr, "Cannot create %s\n", lbuf2);
		(void) unlink(lbuf1);
		exit(1);
	}
	(void) close(fd);
	NRSdbase = newDBname;
}

rexit(val)
{
	char    lbuf1[ENOUGH];
	char    lbuf2[ENOUGH];

	if(val && *newDBname){
		sprintf(lbuf1, "%s.dir", newDBname);
		sprintf(lbuf2, "%s.pag", newDBname);
		(void) unlink(lbuf1);
		(void) unlink(lbuf2);
	}
	exit(val);
}

changedb()
{
	char    lbuf1[ENOUGH];
	char    lbuf2[ENOUGH];
	int     retval = 0;

	dbase_end();
	NRSdbase = savedb;
	sprintf(lbuf1, "%s.dir", newDBname);
	sprintf(lbuf2, "%s.dir", NRSdbase);
	if(rename(lbuf1, lbuf2) < 0){
		fprintf(stderr, "Rename to %s failed\n", lbuf2);
		retval = 1;
	}
	sprintf(lbuf1, "%s.pag", newDBname);
	sprintf(lbuf2, "%s.pag", NRSdbase);
	if(rename(lbuf1, lbuf2) < 0){
		fprintf(stderr, "Rename to %s failed\n", lbuf2);
		retval = 1;
	}
	exit(retval);
}

#ifndef _42

rename(f1, f2)
char    *f1, *f2;
{
	int     retval;

	(void) unlink(f2);
	retval = link(f1, f2);
	(void) unlink(f1);
	return(retval);
}
#endif

#ifndef vax
bzero(cp, size)
register char   *cp;
register size;
{
	do{
		*cp++ = 0;
	}while(--size);
}
#endif
