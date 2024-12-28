/* unix-niftp bin/dbpatch.c $Revision: 5.6.1.2 $ $Date: 1993/01/10 07:02:11 $ */
/*
 * $Log: dbpatch.c,v $
 * Revision 5.6.1.2  1993/01/10  07:02:11  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.5  90/08/01  13:30:08  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:32:20  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:17:25  bin
 * New programme .....
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "opts.h"
#include "nrs.h"
#include <ctype.h>
#include <netinet/in.h>

#ifndef ENOUGH
#define ENOUGH  150
#endif

#include <signal.h>
SIGRET     sigcatch();

char    ibuf[BUFSIZ];
#define	NARGS	20
char	*args[NARGS];
int	fields;

int     oldstyle;

main(argc, argv)
char    **argv;
{
	register char   *p;
	register c;
	static  int     done;
	domin_entry dom;

	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise\n");
		exit(1);
	}
	if(argc > 1 && strcmp(argv[1], "-O") == 0){
		oldstyle++;
		argc--;
		argv++;
	}
	if(argc > 1)                    /* set up the NRSdbase as an arg */
		NRSdbase = argv[1];
	setbuf(stderr, NULL);
	if(signal(SIGHUP, sigcatch) == SIG_IGN)
		(void) signal(SIGHUP, SIG_IGN);
	if(signal(SIGINT, sigcatch) == SIG_IGN)
		(void) signal(SIGINT, SIG_IGN);
	if(signal(SIGQUIT, sigcatch) == SIG_IGN)
		(void) signal(SIGQUIT, SIG_IGN);
	if(signal(SIGTERM, sigcatch) == SIG_IGN)
		(void) signal(SIGTERM, SIG_IGN);
	for(;;done=1){
		for(p = ibuf; (c = getc(stdin)) != EOF && c != '\n';*p++ = c);
		if(c == EOF){
			if(done)
				break;
			fprintf(stderr, "Premature EOF\n");
			break;
		}
		*p = 0;
		if (*ibuf == '#') continue;

		fields = parse(ibuf, ':', args, NARGS);
		switch(args[0][0]){
		case 'h':
			if(host()  == 0)
				continue;
			break;
		case 's':
			if(fields != 3){
				fprintf(stderr, "%d fields instead of 3 for %s\n",
					fields, fields < 2 ? "<unknown>" :
					args[1]);
			}
			else dbase_oenter(args[1], args[2]);
			continue;
			break;
		case 'd':
			dom.domin_name = args[1];
			dom.domin_host = args[2];
			if(fields != 3){
				fprintf(stderr, "%d fields instead of 3 for %s\n",
					fields, fields < 3 ? "<unknown>" :
					args[1]);
			}
			else dbase_denter(&dom);
			continue;
		default:
			fprintf(stderr, "Uknown type `%s'\n", args[0]);
			break;
		}
		break;
	}
	dbase_end();
	exit(0);
}

#define	HFIELDS	13
#define doneok() { if(dbaseenter(h) < 0){ \
		fprintf(stderr, "Enter failed for %s\n", host); \
		return(-1); \
	} \
	else return(0); }

host()
{	struct  host_entry      *h;
	net_entry *netp;
	int	context;
	int	gates;
	char	netc;
	char	*hname	= args[2];
	char	new_[ENOUGH];
	char	*new = (char *) 0;

	if(fields != HFIELDS){
		fprintf(stderr, "%d fields instead of %d for %s\n",
			fields, HFIELDS, fields < 2 ? "<unknown>" :
			(*hname) ? hname : args[1]);
		return(-1);
	}

	/* It is LEGAL to have no short name */
	if (! *hname) hname = args[1];

	/* People may be lazy & omit the long form ... */
	if (! *args[1]) args[1] = hname;

	/* Get the host entry.  If omitted, generate one */
	if (!(h = dbase_get(hname)))
	{	static struct host_entry h_;
		static long t_time = 0;
		static int hostn = 0;

		if (t_time == 0) t_time = htonl(time((long *) 0));
		if (hostn == 0)
		{	int guess;
			char val[20];
			for(guess=1; ;guess *= 2)
			{	sprintf(val, "##%d", guess);
				if (! dbase_get(val)) break;
				if (guess > 100000)
				{	fprintf(stderr, "Too many existing hosts\n");
					return(-1);
				}
			}
			hostn = guess/2;

			while ((guess - hostn) > 1)
			{	int next = (hostn + guess) /2;
				sprintf(val, "##%d", next);
				if (dbase_get(val))
					hostn = next;
				else	guess = next;
			}
			printf("First host is %d\n", hostn);
		}
		h = &h_;
		bzero(h, sizeof (struct host_entry));
		h->host_name	= args[1];
		h->host_alias	= args[2];
		h->n_timestamp	= t_time;
		h->h_number	= ++hostn;
/*		h->n_context	= ????*/
		h->host_info = "DE <not in the NRS>";
		dbase_ienter(h->h_number, hname);
	}

	/* If present, set ``localhost'' */
	if (*args[11]) h->n_localhost = atoi(args[11]);

	/* Done all the host stuff, now to the network specific info */
	/* No net -> all done */
	if (!*args[3]) doneok();

	/* Find the net table .. */
	for (netp = &(h->n_addrs[0]);
		(netp - &(h->n_addrs[0])) < MAXNETS && netp->net_name; netp++)
		if (strcmp(args[3], netp->net_name) == 0) break;

	/* Oh dear, too many nets */
	if ((netp - &(h->n_addrs[0])) >= MAXNETS)
	{
		fprintf(stderr, "New network %s is the %d network !\n",
			args[3], MAXNETS+1);
		return(-1);
	}

	/* First network entry, so create */
	if (! netp->net_name)
	{	netp->net_name = args[3];
		h->n_nets ++;
	}

	/* If present, set the ``max transfers per call'' */
	if (*args[10])
	{	int isint = 1;
		char *p;
		for (p = args[10]; *p; *p++) if (!isdigit(*p)) isint=0;
		if (isint) netp->n_ftptrans = atoi(args[10]);
		else if (args[10][0] == '0'  && args[10][1] == 'x')
			netp->n_ftptrans = xtoi(args[10] +2);
		else
		{	fprintf(stderr,
				"Sorry -- ftp trans '%s' not understood\n",
				args[10]);
		}
	}

	/* Now go onto the context specific info */
	/* No context -> all done */
	if (! *args[4]) doneok();

	context	= atoi(args[4]);
	gates	= atoi(args[9]);
	if (*args[5])
	{	new = new_;
		sprintf(new, (*args[6]) ? "DT %s\nYB %s" : "DT %s", args[5], args[6]);
	}

	switch (context)
	{
	case 1:
		if (new) netp->x29_addr = new;
		if (*args[9])
			fprintf(stderr, "Warning -- x29 gates ignored for %s\n", hname);
		break;
	case 2:
		if (new) netp->ts29_addr = new;
		if (*args[9])
			fprintf(stderr, "Warning -- ts29 gates ignored for %s\n", hname);
		break;
	case 3:
		if (new) netp->ftp_addr = new;
		if (*args[9]) netp->n_ftpgates = gates;
		break;
	case 4:
		if (new) netp->mail_addr = new;
		if (*args[9]) netp->n_mailgates = gates;
		break;
	case 127:
		if (new) netp->news_addr = new;
		if (*args[9]) netp->n_newsgates = gates;
		break;
	case 126:
		if (new) netp->gate_addr = new;
		if (*args[9])
			fprintf(stderr, "Warning -- gateway gates ignored for %s\n", hname);
		break;
	default:
		fprintf(stderr, "Unknown context %d for %s\n",
			context, hname);
		return(-1);
	}

	if(dbaseenter(h) < 0){
		fprintf(stderr, "Enter failed for %s\n", host);
		return(-1);
	}

	/* Now the reverse info */
	if (! *args[7]) return(0);

	new = new_;
	netc = *args[3];	/* HACK HACK HACK How IS the mapping done? */
	sprintf(new, (*args[8]) ? "%c.%s.%s" : "%c.%s", netc, args[7], args[8]);
	dbase_renter(context, new, hname);
	return(0);
}

SIGRET sigcatch()
{
	exit(1);
}

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

parse(line, sep, ptrs, nptrs)
char *line;
char **ptrs;
{	int n = 0;
	ptrs[n++] = line;
	for(; *line; line++) if (*line == sep)
	{	if (n >= nptrs) break;
		*line = '\0';
		ptrs[n++] = line+1;
	}
	return n;
}

xtoi(s)
char *s;
{
	int res = 0;
	for (; *s; s++) switch (*s)
	{	case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8':	case '9':
			res = res*16 + (*s) - '0';	break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			res = res*16 + (*s) - 'A' + 10;	break;
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			res = res*16 + (*s) - 'a' + 10;	break;
		default:
			return res;
	}
	return res;
}
