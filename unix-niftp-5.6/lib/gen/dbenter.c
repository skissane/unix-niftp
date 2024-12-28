/* unix-niftp lib/gen/dbenter.c $Revision: 5.6.1.6 $ $Date: 1993/01/10 07:09:50 $ */
/*
 * $Log: dbenter.c,v $
 * Revision 5.6.1.6  1993/01/10  07:09:50  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:00:32  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.4  89/08/26  15:40:03  pb
 * Distribution of Aug89PPsupport: Update READMEs for PP
 * 
 * Revision 5.1  88/10/07  17:13:57  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:26:28  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:03:44  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/07/10  15:28:29  pb
 * lowercase on input.
 * 
 * Revision 5.0  87/03/23  03:35:28  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include <ctype.h>
#include "opts.h"
#include "db.h"
#include "nrs.h"

static  char    ebuf[BUFSIZ];
static  int     ebuf_len;
static  char    adbuf[BUFSIZ/2];
static  int     adbuf_len;
static  char    albuf[BUFSIZ/2];
static  int     albuf_len;
static  datum   dbin, dbdat;

char    *strcpy(), *strcat();
static  char    *Itos();

dbaseenter(hp)
register struct  host_entry *hp;
{
	register char   *p, *q;
	register net_entry *np;
	int     k;

	if(!_dbase_inuse && dbase_start()< 0)
		return(-1);
	if(hp->host_name == NULL || hp->host_alias == NULL)
		return(-1);
	(void) lowerify(adbuf+1, hp->host_name);
	adbuf_len = strlen(adbuf+1) + 1;
	*adbuf = '>';                   /* '>' for alias entry */
	(void) lowerify(albuf, hp->host_alias);
	albuf_len = strlen(albuf) + 1;
	*ebuf = '#';                    /* hash for host entry */
	(void) lowerify(ebuf+1, albuf);
	p = &ebuf[albuf_len + 1];       /* character beyond alias */
	*p++ = DBSEP;
	if((q = hp->host_info) != NULL)
		while((*p++ = *q++) != NULL) continue;
	*p++ = DBSEP;
	if(hp->n_nets > MAXNETS){
		printf("n_nets too big %d for %s\n",hp->n_nets,hp->host_name);
		hp->n_nets = MAXNETS;
	}
	*p++ = hp->n_nets;
	/* *p++ = DBSEP; */
	for(q = Itos(hp->h_number); *q ; )
		*p++ = *q++;
	*p++ = DBSEP;
	q = (char *)&hp->n_timestamp;
	*p++ = *q++;
	*p++ = *q++;
	*p++ = *q++;
	*p++ = *q++;
	*p++ = hp->n_localhost;
	*p++ = hp->n_oldhost;
	*p++ = hp->n_disabled;
	*p++ = DBSEP;
	for(k = 0; k < hp->n_nets ; k++){
		np = &hp->n_addrs[k];
		if((q = np->net_name) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
		if((q = np->ts29_addr) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
		if((q = np->x29_addr) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
		if((q = np->ftp_addr) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
		for(q = Itos(np->n_ftpgates); *q ;)
			*p++ = *q++;
		*p++ = DBSEP;
		if((q = np->mail_addr) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
		if((q = np->jtmp_addr) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
#ifdef NEWS
		if((q = np->news_addr) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
#endif NEWS
		for(q = Itos(np->n_mailgates); *q ;)
			*p++ = *q++;
		*p++ = DBSEP;
		for(q = Itos(np->n_ftptrans); *q ;)
			*p++ = *q++;
		*p++ = DBSEP;
		for(q = Itos(np->n_jtmpgates); *q ;)
			*p++ = *q++;
		*p++ = DBSEP;
#ifdef NEWS
		for(q = Itos(np->n_newsgates); *q ;)
			*p++ = *q++;
		*p++ = DBSEP;
#endif NEWS
		if((q = np->gate_addr) != NULL)
			while((*p++ = *q++) != NULL) continue;
		*p++ = DBSEP;
	}
	ebuf_len = p - ebuf;
	dbin.dptr = adbuf+1;
	dbin.dsize = adbuf_len;
	dbdat.dptr = ebuf;
	dbdat.dsize = ebuf_len;
	if(lc_store(dbin, dbdat) < 0){
		fprintf(stderr, "Write error on data base\n");
		return(-1);
	}
	if(strcmp(hp->host_name, hp->host_alias) == 0)
		return(0);
	dbin.dptr = albuf;
	dbin.dsize = albuf_len;
	dbdat.dptr = adbuf;
	dbdat.dsize = adbuf_len +1;
	if(lc_store(dbin, dbdat) < 0){
		fprintf(stderr, "Write error on alias\n");
		return(-1);
	}
	return(0);
}

static  char    *
Itos(numb)
register numb;
{
	static  char    x[10];
	register char   *p;

	p = &x[9];
	*p-- = 0;
	do{
		*p-- = numb%10 + '0';
	}while(numb /= 10);
	return(++p);
}

/*
 * Add a reverse ts string to the Dbase
 */

static  Xcompress();

dbase_renter(context, tsstring, name)
char    *tsstring, *name;
{
	register char   *p = ebuf;

	if(!_dbase_inuse && dbase_start() < 0)
		return(-1);
	*p++ = '$';
	*p++ = context;
	(void) lowerify(p, name);
	Xcompress(tsstring, adbuf);

	dbin.dptr = adbuf;
	dbin.dsize = strlen(adbuf)+1;
	dbdat.dptr = ebuf;
	dbdat.dsize = strlen(ebuf+2)+3;
	if(lc_store(dbin, dbdat) < 0){
		fprintf(stderr, "Write error on rtrans\n");
		return(-1);
	}

	p = adbuf;
	sprintf(p = adbuf, "c%d.", context);
	while (*p) p++;
	Xcompress(tsstring, p);

	dbin.dsize = strlen(adbuf)+1;
	if(lc_store(dbin, dbdat) < 0){
		fprintf(stderr, "Write error on rtrans\n");
		return(-1);
	}
	return(0);
}

/*
 * delete any leading zero's + convert to standard form + do a copy.
 */

static
Xcompress(from, to)
register char   *from, *to;
{
#ifdef NOLEADINGZEROS
	register char *p;
	int zeros = 0, rest = 0;

	if((from[0] == 'j' || from[0] == 'p') && from[1] == '.'){
		*to++ = *from++;
		*to++ = *from++;
		/* check length of address */
		p = from;
		while(*p == '0')
			zeros++, p++;
		while('0' <= *p && *p <= '9')
			rest++, p++;
		/* suppress leading zeros unless subaddress */
		if(zeros + rest <= 12)
			from += zeros;
	}
#endif NOLEADINGZEROS
	while((*to++ = *from++) != NULL) continue;
}

dbase_ienter(number, name)
char    *name;
{
	register char   *p, *q;
	static  char    x[10] = "##";

	if(!_dbase_inuse && dbase_start() < 0)
		return(-1);
	for(p = Itos(number), q = x+2; (*q++ = *p++) != NULL;) continue;
	return(dbase_oenter(x, name));
}

dbase_oenter(from, to)
char    *from, *to;
{
	if(!_dbase_inuse && dbase_start()< 0)
		return(-1);
	if(from == NULL || to == NULL)
		return(-1);
	if(strcmp(from, to) == 0)       /* don't add aliases to self */
		return(0);
	(void) lowerify(adbuf+1, to);
	*adbuf = '>';                   /* '>' for alias entry */
	dbin.dptr = from;
	dbin.dsize = strlen(from) +1;
	dbdat.dptr = adbuf;
	dbdat.dsize = strlen(adbuf+1) + 2;
	if(lc_store(dbin, dbdat) < 0){
		fprintf(stderr, "Write error in dbase_oenter\n");
		return(-1);
	}
	return(0);
}

/*
 * enter a domain entry into the database
 */
dbase_denter(dom)
register domin_entry *dom;
{
	if(!_dbase_inuse && dbase_start()< 0)
		return(-1);
	if(dom->domin_name == NULL || dom->domin_host == NULL)
		return(-1);
	(void) lowerify(adbuf+1, dom->domin_host);
	*adbuf = '@';                   /* '@' for domain entry */
	dbin.dptr = dom->domin_name;
	dbin.dsize = strlen(dom->domin_name) +1;
	dbdat.dptr = adbuf;
	dbdat.dsize = strlen(adbuf+1) + 2;
	if(lc_store(dbin, dbdat) < 0){
		fprintf(stderr, "Write error in dbase_denter\n");
		return(-1);
	}
	return(0);
}

dbase_Denter(number, name)
char    *name;
{
	register char   *p, *q;
	static  char    x[10] = "@@";

	if(!_dbase_inuse && dbase_start() < 0)
		return(-1);
	for(p = Itos(number), q = x+2; (*q++ = *p++) != NULL;) continue;
	return(dbase_oenter(x, name));
}

lc_store(dbin, dbdat)
datum   dbin, dbdat;
{	datum	lc_dbin;
	register char dummy[BUFSIZ];
	register char *data = dbin.dptr;
	register i;

	if (dbin.dsize > sizeof (dummy))
	{	int upper = 0;

		for (i=0; i<dbin.dsize; i++) if (isupper(data[i])) upper++;

		if (upper)
		 fprintf(stderr, "Key `%.*s' > %d chars so not lower cased\n",
				dbin.dsize, data, sizeof(dummy));
		return store(dbin, dbdat);
	}

	/* THER SHOULD BE A MACRO/PROCEDURE TO DO THIS !!! */
	for (i=0; i<dbin.dsize; i++)
		dummy[i] = (isupper(data[i])) ? data[i] + 'a' - 'A' : data[i];

	lc_dbin.dptr	= dummy;
	lc_dbin.dsize	= dbin.dsize;
	return store(lc_dbin, dbdat);
}

lowerify(to, from)
char *to;
char *from;
{	for (;*from; from++) *to++ = (isupper(*from)) ? tolower(*from) : *from;
	*to = '\0';
}
