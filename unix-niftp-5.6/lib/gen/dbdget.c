/* unix-niftp lib/gen/dbdget.c $Revision: 5.6.1.6 $ $Date: 1993/01/10 07:09:47 $ */
/*
 * $Log: dbdget.c,v $
 * Revision 5.6.1.6  1993/01/10  07:09:47  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:00:30  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:12  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:42:57  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:35:25  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "db.h"
#include "nrs.h"
#include "log.h"
extern ftp_print;

#ifndef ENOUGH
#define ENOUGH  150
#endif

static  domin_entry     dom;
static  char    dombuf[ENOUGH];

extern char *index();

domin_entry *
domin_get(name)
char    *name;
{
	register char   *p, *q, *r;
	datum   fkey;
	datum   dat;
	char    xbuf[BUFSIZ];
	int     once;

	if(!_dbase_inuse && dbase_start() < 0)
		return(0);
	for(p = name, q = xbuf ; (*q = *p++) != NULL ; q++)
		if(*q >= 'A' && *q <= 'Z')
			*q += 'a' - 'A';
	fkey.dptr = xbuf;
	fkey.dsize = p - name;

	dat = fetch(fkey);
	if( (p = dat.dptr) == 0)
		return(0);
	for(once = 0; *p != '@' ; once++){
		switch(*p){
		case '>':
			for(q = xbuf, r = dat.dptr+1 ; (*q++ = *r++) != NULL;) continue;
			fkey.dptr = xbuf;
			fkey.dsize = dat.dsize-1;
			dat = fetch(fkey);
			if((p = dat.dptr) == 0)
				return(0);
			if(*p == '@' || !once)
				continue;
			fprintf(stderr,"corrupt entry for %s\n", name);
			return(0);
		case '$':
		case '#':
			/*
			 * a host entry - ignore
			 */
			return(0);
		default:
			/*
			 * for now just report error
			 */
			fprintf(stderr, "bad key for %s <%d>\n",
							name,*p&0377);
			return(0);
		}
	}
	dom.domin_name = q = dombuf;
	for(r = xbuf ; (*q++ = *r++) != NULL;) continue;   /* copy over */
	for(dom.domin_host = q, p++; (*q++ = *p++) != NULL;) continue;
	return(&dom);
}

static int matches(string, skip, address)
char	*string;
int	skip;
char	*address;
{
	if (!string) return 0;

	L_DEBUG_3(L_80, 0, "dbdget.matches(%s, %d, %s)\n", string, skip, address);
	if (string && (*string == '@')) string++;
	while (string && *string)
	{	char *this = string;
		int len;
		string = index(string, '|');

		if (string) len = string++ - this;
		else len = strlen(this);

		L_DEBUG_4(L_80, 0, "dbdget.matches: try %d `%*.*s'",
			len-skip, len-skip, len-skip, this+skip);
		L_DEBUG_3(L_80, L_CONTINUE, "and `%*.*s'\n",
			len-skip, len-skip, address+skip);
		if (!strncasecmp(this+skip, address+skip, len-skip)) return 1;
	}
	L_DEBUG_3(L_80, 0, "dbdget.matches(%s, %d, %s) fails\n", string, skip, address);
	return 0;
}

int nsap_validate(nsap, net, address)
char	*nsap;
int	net;
char	*address;
{
	datum   fkey;
	datum   dat;
	int	len;
	int	skip = 0;

	char	xbuf[BUFSIZ];
	char	fulladdr[BUFSIZ];

	if(!_dbase_inuse && dbase_start() < 0) return(0);

	if (net <= 0) skip++;
	fulladdr[0] = (skip) ? '*' : net;
	fulladdr[1] = '.';
	strcpy(fulladdr +2, address);
	
	strcpy(xbuf, "nsap=");
	strcat(xbuf, nsap);
	len = strlen(xbuf) +1;

	fkey.dptr = xbuf;
	fkey.dsize = len;
	dat = fetch(fkey);
	L_DEBUG_2(L_80, 0, "dbdget.nsap_validate: `%s' gave `%s'\n", xbuf, dat.dptr);
	if (matches(dat.dptr, skip, fulladdr)) return 1;

	xbuf[4] = '*';
	while (len-- > 5)
	{	fkey.dptr[len] = '\0';
		fkey.dsize = len+1;
		dat = fetch(fkey);
		L_DEBUG_3(L_80, 0, "len=%d: `%s', '%s'\n",
			len, fkey.dptr, dat.dptr);
		if (matches(dat.dptr, skip, fulladdr)) return 1;
	}
	L_DEBUG_0(L_80, 0, "dbdget.nsap_validate: total failure\n");
	return 0;
}
