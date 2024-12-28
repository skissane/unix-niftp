/* unix-niftp bin/lookdbm.c $Revision: 5.6.1.2 $ $Date: 1993/01/10 07:00:48 $ */
/*
 * look up entries in the data base
 *
 * $Log: lookdbm.c,v $
 * Revision 5.6.1.2  1993/01/10  07:00:48  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.5  90/08/01  13:31:52  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.4  89/08/26  15:46:50  pb
 * Distribution of Aug89PPsupport: Update READMEs for PP
 * 
 * Revision 5.1  88/10/07  17:03:18  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  17:05:41  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  17:04:06  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 1.1  87/10/30  19:11:51  pb
 * Initial revision
 * 
 * Revision 5.0  87/03/23  03:19:17  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "opts.h"
#include "nrs.h"
#define clean(s, def) s ? hide_pss_pw(s) : def
/*#define clean(s, def) s ? cleanup(s) : def*/
char *index();
char *hide_pss_pw();
char  lreverse, lcomplain;
char *xfer_type;
struct host_entry *nrs_dtetoname();

#ifndef	FIXED_NET_FAIL
net_fail() { return; }
#endif

int all = 0;
int raw = 0;

main(argc,argv)
char    **argv;
{
	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise\n");
		exit(1);
	}
	argv++;
	while(argc-- > 1)
		print_info(*argv++);
	exit(0);
}


print_addr(text)
char *text;
{	for (; *text; text++)
	{	putchar(*text);
		if ((*text == '\n') && text[1]) printf("             ");
	}
}

char *
cleanup(s)
char *s;
{	char *p;
	if (!s || !index(s, '(')) return s;

	p = s;
	while(p)
	{	if (strncmp(p, "YB", 2) == 0 && (p = (index(p, '('))))
		{	char *e = index(p, ')');
			if (e) strcpy(p+1, e);
		}
		p = index(p+1, '\n');
	}
	return s;
}

print_val(key, val, def, cl)
char *key;
char *val;
char *def;
{
	if (val || all)
	{	printf("%s:%*s", key, 12-strlen(key), "");
		print_addr((val) ? (cl) ? cleanup(val) : val : def);
		printf("\n");
	}
}

print_info(host)
char    *host;
{
	struct  host_entry      *hp;
	int     i;
	net_entry       *np;
	char    *ctime();
	domin_entry     *dp;
	static	printed = 0;

	if (!strcmp(host, "-a")) { all++; return; }
	if (!strcmp(host, "+a")) { all=0; return; }
	if (!strcmp(host, "-r")) { raw++; return; }

	if (printed == 0) printed++;
	else printf("\n");
	if (raw) return deb_raw(host);

	hp = dbase_get(host);
	if(hp == NULL){
		char sbuf[1024];
		if (dp = domin_get(host))
		{	printf("%s is a domain entry:- %s %s\n",
				host, dp->domin_name, dp->domin_host);
			return;
		}
		/*
		 * finally check possible domainless references
		 */
		hp = dbase_find(host, sbuf, sizeof sbuf);

		/*
		 * give up. Cannot find host in tables
		 */
	badhost:;

		if(hp == NULL){
			if (!(hp = nrs_dtetoname(host, 0)))
			{	fprintf(stderr, "Cannot find entry for %s\n", host);
				return;
			}
			fprintf(stderr, "%s not found -- assuming dte\n", host);
		}
		else fprintf(stderr, "%s not found -- assuming host %s\n",
			host, sbuf);
	}

	printf("Info for:    %s:-\n", host);
	print_val("Host Name", hp->host_name, "NO NAME", 1);
	print_val("Alias", hp->host_alias, "NO ALIAS", 1);
	print_val("Host Info", hp->host_info, "NO INFO", 1);
	if (all || hp->n_timestamp)
		printf("Added:       %s",hp->n_timestamp ? ctime(&hp->n_timestamp) :
								"NEVER\n");
	printf("Num of nets: %d\n", hp->n_nets);
	if(hp->n_context != -1)
		printf("Context:     %d\n", hp->n_context);
	printf("Host Number: %d\n", hp->h_number);
	if(hp->n_localhost)
		printf("local host\n");
	if(hp->n_oldhost)
		printf("from old host file\n");
	if(hp->n_disabled)
		printf("host disabled\n");
	for(i = 0 ; i < hp->n_nets ; i++ ){
		np = &hp->n_addrs[i];
		printf("Network:     %s\n", clean(np->net_name, "NO NET"));
		if(np->ftp_addr ||
#ifdef NEWS
			np->news_addr ||
#endif NEWS
			np->mail_addr)
		{	if(np->n_ftptrans & ~ TRANS_MAX_MASK)
			{	printf("FTP trans:   %x\n", np->n_ftptrans);
				if(TRANS_MAX_VAL(np->n_ftptrans) != 50)
				    printf("  Max trans/TS conn: %d\n",
					TRANS_MAX_VAL(np->n_ftptrans));
				if(TRANS_WIND_VAL(np->n_ftptrans))
				    printf("  Window size:       %d\n",
					TRANS_WIND_VAL(np->n_ftptrans));
				if(TRANS_PKTS_VAL(np->n_ftptrans))
				    printf("  Pkt size:          %d (%d)\n",
					8 << TRANS_PKTS_VAL(np->n_ftptrans),
					TRANS_PKTS_VAL(np->n_ftptrans));
				if(TRANS_CALL_VAL(np->n_ftptrans))
				    printf("  %s\n",
			(TRANS_CALL_VAL(np->n_ftptrans) == TRANS_CALL_GW1) ?
					"Call using Gateway 1" :
			(TRANS_CALL_VAL(np->n_ftptrans) == TRANS_CALL_GW2) ?
					"Call using Gateway 2" :
			(TRANS_CALL_VAL(np->n_ftptrans) == TRANS_CALL_NEVER) ?
					"Never call (is polled)" :
					"??");
				if(TRANS_CUG_VAL(np->n_ftptrans))
				    printf("  Closed User Group:  %02d\n",
					TRANS_CUG_VAL(np->n_ftptrans));
				if (TRANS_REVC_VAL(np->n_ftptrans))
				    printf("  Reverse charged\n");
				if (TRANS_UNIXNIFTP_VAL(np->n_ftptrans))
				    printf("  Unix-Niftp host\n");
				if (TRANS_NFCS_VAL(np->n_ftptrans))
				    printf("  No Fast Call Select\n");
				if (TRANS_FCS_VAL(np->n_ftptrans))
				    printf("  Fast Call Select\n");
				if(TRANS_UNUSED_VAL(np->n_ftptrans))
				    printf("  Undefined mask:    %x\n",
					TRANS_UNUSED_VAL(np->n_ftptrans));
			}
			else if (TRANS_MAX_VAL(np->n_ftptrans) != 50)
                            printf("Max trans/TS conn: %d\n",
                                TRANS_MAX_VAL(np->n_ftptrans));
		}
		else if (TRANS_MAX_VAL(np->n_ftptrans) != 50)
			printf("Xfer id:     %x\n",
			    TRANS_MAX_VAL(np->n_ftptrans));
		print_val("Ts29", np->ts29_addr, "NO TS29", 1);
		print_val("X29", np->x29_addr, "NO X29", 1);
		print_val("Niftp", np->ftp_addr, "NO NIFTP", 1);
		if(np->ftp_addr && np->n_ftpgates)
				printf("Ftpgateways: %d\n", np->n_ftpgates);
		print_val("Mail", np->mail_addr, "NO MAIL", 1);
		if(np->mail_addr && np->n_mailgates)
			printf("Mailgateways: %d\n", np->n_mailgates);
		print_val("Jtmp", np->jtmp_addr, "NO JTMP", 1);
		if(np->jtmp_addr && np->n_jtmpgates)
			printf("Jtmpgateways: %d\n", np->n_jtmpgates);
#ifdef NEWS
		print_val("News", np->news_addr, "NO NEWS", 1);
		if(np->news_addr && np->n_newsgates)
			printf("Newsgateways: %d\n", np->n_newsgates);
#endif NEWS
		print_val("Gate", np->gate_addr, "NO GATEWAY", 1);
	}
}

#include <ctype.h>
#include "dbm.h"
extern _dbase_inuse;

deb_raw(host)
char *host;
{
	datum   fkey;
	char *p, *q;
	datum   dat;
	char    xbuf[BUFSIZ];

	if(!_dbase_inuse && dbase_start() < 0)
		return(0);
	for(p = host, q = xbuf ; *q = *p++ ; q++)
		if(*q >= 'A' && *q <= 'Z')
			*q += 'a' - 'A';
	fkey.dsize = p - host;
	fkey.dptr  = xbuf;

	dat = fetch(fkey);
	if( (p = dat.dptr) == 0)
	{	printf("Lookup of `%s' gave %d\n", xbuf, p);
		return(0);
	}
	printf("Lookup of `%s' gave %d: '", xbuf, dat.dsize);
	for(;dat.dsize-- > 0; p++)
		printf((ispunct(*p) || isalnum(*p) || *p == ' ') ? "%c" : "[%02x]", (unsigned char) *p);
	printf("'\n");
}
