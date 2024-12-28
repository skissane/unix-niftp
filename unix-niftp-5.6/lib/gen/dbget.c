/* unix-niftp lib/gen/dbget.c $Revision: 5.6 $ $Date: 1991/06/07 17:00:17 $ */
/*
 * $Log: dbget.c,v $
 * Revision 5.6  1991/06/07  17:00:17  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:19  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.4  89/08/26  15:39:59  pb
 * Distribution of Aug89PPsupport: Update READMEs for PP
 * 
 * Revision 5.3  89/07/16  12:02:57  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:43:53  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:16:46  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0  87/03/23  03:35:36  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "opts.h"
#include "db.h"
#include "nrs.h"

#ifndef	ntohl
#include <netinet/in.h>	/* Let me know if this needs to be conditional */
#endif ntohl

static  char    fbuf[BUFSIZ];
static  struct  host_entry h;
char    *strncpy();

#ifdef  STDV7
typedef int     void;
#endif

struct  host_entry *
dbase_get(host)
char    *host;
{
	register char   *p, *q, *r;
	register int    s;
	register net_entry *np;
	int     S, once, ctex;
	datum   fkey;
	datum   dat;
	char    xbuf[BUFSIZ];

	if(!_dbase_inuse && dbase_start() < 0)
		return(0);
	for(p = host, q = xbuf ; (*q = *p++) != NULL ; q++)
		if(*q >= 'A' && *q <= 'Z')
			*q += 'a' - 'A';
	fkey.dptr = xbuf;
	fkey.dsize = p - host;

	dat = fetch(fkey);
	if( (p = dat.dptr) == 0)
		return(0);
	for(ctex = -1, once = 0; *p != '#' ; once++){
		switch(*p){
		case '>':
			for(q = xbuf, r = dat.dptr+1 ; (*q++ = *r++) != NULL;) continue;
			fkey.dptr = xbuf;
			fkey.dsize = dat.dsize-1;
			dat = fetch(fkey);
			if((p = dat.dptr) == 0)
				return(0);
			if(*p == '#' || !once)
				continue;
			fprintf(stderr,"corrupt entry for %s\n", host);
			return(0);
		case '$':
			/*
			 * it's a reverse translate
			 */
			ctex = p[1] & 0377;
			for(q = xbuf, r = dat.dptr+2 ; (*q++ = *r++) != NULL;) continue;
			fkey.dptr = xbuf;
			fkey.dsize = dat.dsize-2;
			dat = fetch(fkey);
			if((p = dat.dptr) == 0)
				return(0);
			if(*p == '#' || !once)
				continue;
			fprintf(stderr,"corrupt entry for %s\n",host);
			return(0);
		case '@':
			/*
			 * it's a domain entry - return null
			 */
			return(0);
		default:
			/*
			 * for now just report error
			 */
			fprintf(stderr, "bad key for host %s <%d>\n",
							host,*p&0377);
			return(0);
		}
	}
	p++;

	bzero( (char *)&h, sizeof(h)); /* clear the host structure */

	h.host_name = q = fbuf;
	h.n_context = ctex;
					/* copy across the host name */
	for(r = xbuf; (*q++ = *r++) != NULL;) continue;
	if(*p != DBSEP){
		h.host_alias = q;
		while(*p != DBSEP)
			*q++ = *p++;
	}
	if(*++p != DBSEP){
		h.host_info = q;
		while(*p != DBSEP)
			*q++ = *p++;
	}
	h.n_nets = *++p;
	if( (unsigned) h.n_nets > MAXNETS){
		fprintf(stderr, "Out of bound n_nets %d\n", h.n_nets);
		return(0);
	}
	for(s = 0, ++p ; *p != DBSEP ; s = s*10 + (*p++ - '0')) continue;
	h.h_number = s;
	p++;
	/* unpack the time */
	r = (char *)&h.n_timestamp;
	*r++ = *p++;
	*r++ = *p++;
	*r++ = *p++;
	*r++ = *p++;
	h.n_timestamp = ntohl(h.n_timestamp);
	h.n_localhost = *p++;
	h.n_oldhost = *p++;
	h.n_disabled = *p++;
	for(S = 0, np = h.n_addrs; S < h.n_nets ; S++, np++){
		if(*++p != DBSEP){
			np->net_name = q;
			while(*p != DBSEP)
				*q++ = *p++;
		}
		if(*++p != DBSEP){
			np->ts29_addr = q;
			while(*p != DBSEP)
				*q++ = *p++;
		}
		if(*++p != DBSEP){
			np->x29_addr = q;
			while(*p != DBSEP)
				*q++ = *p++;
		}
		if(*++p != DBSEP){
			np->ftp_addr = q;
			while(*p != DBSEP)
				*q++ = *p++;
		}
		if(*++p != DBSEP){
			for(s = 0 ; *p != DBSEP ; s = s*10 + (*p++ - '0')) continue;
			np->n_ftpgates = s;
		}
		if(*++p != DBSEP){
			np->mail_addr = q;
			while(*p != DBSEP)
				*q++  = *p++;
		}
		if(*++p != DBSEP){
			np->jtmp_addr = q;
			while(*p != DBSEP)
				*q++  = *p++;
		}
#ifdef NEWS
		if(*++p != DBSEP){
			np->news_addr = q;
			while(*p != DBSEP)
				*q++  = *p++;
		}
#endif NEWS
		if(*++p != DBSEP){
			for(s = 0 ; *p != DBSEP ; s = s*10 + (*p++ - '0')) continue;
			np->n_mailgates = s;
		}
		if(*++p != DBSEP){
			for(s = 0 ; *p != DBSEP ; s = s*10 + (*p++ - '0')) continue;
			np->n_ftptrans = s;
		}
		if(*++p != DBSEP){
			for(s = 0 ; *p != DBSEP ; s = s*10 + (*p++ - '0')) continue;
			np->n_jtmpgates = s;
		}
#ifdef NEWS
		if(*++p != DBSEP){
			for(s = 0 ; *p != DBSEP ; s = s*10 + (*p++ - '0')) continue;
			np->n_newsgates = s;
		}
#endif NEWS
		if(p < dat.dptr + dat.dsize -1 && *++p != DBSEP){
			np->gate_addr = q;
			while(*p != DBSEP && p < dat.dptr + dat.dsize)
				*q++  = *p++;
			if (q[-1] != DBSEP) *q++ = DBSEP;
		}
	}
	if(p > dat.dptr + dat.dsize ){
		fprintf(stderr, "Size wrong for host %d\n", fbuf);
		return(NULL);
	}
	return(&h);
}
