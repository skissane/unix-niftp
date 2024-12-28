/* unix-niftp sendmail/ni_send.c $Revision: 5.5 $ $Date: 90/08/01 13:39:50 $ */
/*
 *  ni_send  -  Sendmail -> NIFTP-MAIL mailer interface.
 *
 *	this program takes a hostname and a list of addresses to send to as
 *	arguments and a mail message as standard input and creates a spool file
 *	containing a Janet header followed by the mail message, then creates
 *	the queue control file and signals the ftp spooler to send the file.
 *
 *	Much code is borrowed from cpf and uk-sendmail's hhsend.
 *
 *	Jim Crammond	<jim@hw.cs>	6/86
 *
 *	Rearranged to call on the unix-niftp interface cpf rather than
 *	duplicate its inards.
 *
 *	William Armitage <wja@nott.cs>  5/2/87
 *
 *	$Log:	ni_send.c,v $
 * Revision 5.5  90/08/01  13:39:50  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:58:14  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:28:23  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:36:13  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.5  88/01/29  12:48:28  pb
 * Distribution of Jan88ReleaseMod1: JRP fixes - tcccomm.c ftp.c + news sucking rsft.c + makefiles
 * 
 * Revision 5.0.1.4  88/01/28  06:14:35  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.2  87/09/28  13:08:46  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:01:32  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  04:01:51  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include <stdio.h>
#include <signal.h>
#include <pwd.h>
#include <sysexits.h>
#include "opts.h"
#include "nrs.h"

#define BUF  256		/* enough space for strings */

/* These are usually passed over by Makefile.real */
#ifndef	CPF
#define	CPF		"/usr/bin/cpf"
#endif	CPF
#ifndef	MAILMOD
#define	MAILMOD		0600	/*  mode of created mail file  */
#endif	MAILMOD
#ifndef	MAILSP
#define	MAILSP		MAILDIR	/*  where to create mail file  */
#endif	MAILSP

char	cpf[] = CPF;
char	mail_file[BUF];		/* name of mail spool file */
char	*from_addr;		/*  address to return errors to  */
short	realuid, realgid;	/*  real user id and group id  */

char	*strcpy(), *strcat(), *index(), *rindex();
struct	host_entry *get_host_info();
char	*relay();
int	abort();
extern	errno;

main(argc,argv)
int	argc;
char	*argv[];
{
	char	*myname;
	char	*host;
	char	**users;
	int	nusers;
	struct  host_entry *hostentp;
	char	route[BUF];		/* record relays */
	char	*ar;

	myname = argv[0];

	if (argc > 1 && argv[1][0] == '-')
	{	if (argv[1][1] == 'f')
		{	from_addr = argv[2];
			argc -= 2;
			argv += 2;
		}
	}

	if (argc < 3)
	{	fprintf(stderr, "usage: %s [-f from] host users...\n", myname);
		exit(EX_USAGE);
	}

	host = argv[1];
	users = &argv[2];
	nusers = argc - 2;
	realuid = getuid();
	realgid = getgid();

	if (nrs_init() < 0)
	{	fprintf(stderr, "%s: Cannot initialise nrs\n", myname);
		exit(EX_SOFTWARE);
	}

	/*
	 * lookup host in tables.
	 */
	hostentp = get_host_info(host);

	if (hostentp == NULL)
	{	fprintf(stderr, "Unknown host %s\n", host);
		exit(EX_NOHOST);
	}
	/* Put the canonical address in the route.
	 * This ensures the result is fully qualified AND that there the host
	 * name is explicitly mentioned (may get "user" rather than "user@mc")
	 */
	strcpy(route, "%");
	strcat(route, hostentp->host_name);

	/*
	 * Check for application relays here.
	 * If mail_addr contains an AR entry we look that up (recursively).
	 * Thus hostentp ends up pointing at the host we actualy
	 * deliver the message to.
	 * Also to stick to the Grey Book recomendations the
	 * Grey Book header addresses have the resultant
	 * route munged into them.
	 */
	while(ar = relay(hostentp))
	{	strcat(route,"%");
		strcat(route,ar);
		if(!(hostentp = get_host_info(ar)))
		{	fprintf(stderr,"Unknown application relay %s\n",ar);
			exit(EX_NOHOST);
		}
	}
	*(rindex(route,'%')) = '@';

	/*	catch signals	*/
	(void) signal(SIGHUP, abort);
	(void) signal(SIGINT, abort);
	(void) signal(SIGQUIT, abort);
	(void) signal(SIGTERM, abort);

#ifndef	MAILOWNSMAIL
	if (from_addr)
	{	char	name[9];
		char *at = index(from_addr, '@');
		int len = (at) ? at - from_addr : strlen(from_addr);
		struct  passwd  *cur_user = (struct passwd *) 0;
		struct  passwd  *getpwnam();

		if (len < sizeof name)
		{	bzero(name, sizeof name);
			strncpy(name, from_addr, len);	
			if (cur_user = getpwnam(name))
				realuid = cur_user->pw_uid;
		}
	}
#endif	MAILOWNSMAIL

	/* create JNT mail file */
	create_mail_file(users, nusers, route);

	/* pass down to the ftp */
	exit(tellftp(hostentp->host_name, from_addr, mail_file));
}


struct host_entry *
get_host_info(host)
char	*host;
{
	register char   **xp;
	struct	host_entry *dbp;
	char	sbuf[BUF];
	char	domain[BUF];
	char *p;

	/*
	 *  handle domain literals  (not yet)
	 */
	
	strcpy(domain, host);

	for (;;)
	{
		/*
		 * check for exact match on domain
		 */
		if ( (dbp = dbase_get(domain)) != NULL)
			break;

		/*
		 *  check possible abbreviated domain names
		 */
		for (xp = NRSdomains; dbp == NULL && *xp; xp++)
		{
			(void) strcpy(sbuf, *xp);
			(void) strcat(sbuf, domain);
			dbp = dbase_get(sbuf);
		}
		if (dbp)
			break;
		/*
		 * strip another subdomain if there is one else give up
		 */
		if ((p = rindex(domain, '.')) == NULL)
			break;
		else
			*p = '\0';
	}
	if (dbp == NULL || dbp->n_context != -1)
		return(NULL);
	return dbp;
}

create_mail_file(users, nusers, route)
char	*users[];
int	nusers;
char	*route;
{
	char    line[BUFSIZ];
	FILE	*mailf;

	/* This is a bad choice -- the name may be re-used */
	sprintf(mail_file, "%s/m.%05d",
#ifdef SENDMAIL_DEBUG
		".",
#else
		MAILSP,
#endif
		getpid());

	mailf = fopen(mail_file, "w");
	if (mailf == NULL)
	{	fprintf(stderr, "can't create mail spool file %s\n", mail_file);
		abort(EX_CANTCREAT);
	}

	/*  file must be owned by the user;  make it readable only by him */
	chown(mail_file, realuid, realgid);
	chmod(mail_file, MAILMOD);

	/*  write out JNT header  */
	while (nusers > 0)
	{	char *p;
		char *auser = users[ --nusers ];
		if (*route && (p=index(auser, '@'))) *p = '\0';
		fprintf(mailf, "%s%s%c\n",
			auser, route, (nusers) ? ',' : '\n');
	}

	/*  write out header + message from stdin (supplied by sendmail)  */
	while (fgets(line, sizeof(line), stdin) != NULL)
		fputs(line, mailf);
	
	if (ferror(mailf) || fclose(mailf) == EOF)
	{	fprintf(stderr, "Write failure on %s (%d)\n",
			mail_file, errno);
		abort(EX_CANTCREAT);
	}
}

/*
 * pass mail file to the ftp.
 * does any exit status translation required
 *
 * apart from the existance of the database this is the only ftp
 * dependent part of this program.
 */
tellftp(host, from, file)
char *host, *from, *file;
{
	char *args[10];
	char frombuf[BUF];
	char hostbuf[BUF];
	char *p;
	int i, pid, status;

	while ((pid = fork()) < 0)
		sleep(30);

	if (pid == 0) { /* child - invoke cpf */
		i = 0;
		if (p = rindex(cpf, '/'))
			p++;
		else
			p = cpf;
		args[i++] = p;
		args[i++] = "-t";
		if (from) {
			sprintf(frombuf, "-f%s", from);
			args[i++] = frombuf;
		}
		args[i++] = file;
	      /*sprintf(hostbuf, "@%s/%s", net, host); */
		sprintf(hostbuf, "@%s", host);
		args[i++] = hostbuf;
		args[i] = (char *)0;
#ifdef SENDMAIL_DEBUG
		printf("command:");
		for (i = 0; args[i]; i++)
			printf(" %s", args[i]);
		printf("\n");
		exit(0);
#endif SENDMAIL_DEBUG

#ifdef SETEUID
		setreuid(0, -1);
#else
		setuid(0);
#endif SETEUID
		execv(cpf, args);
		exit(5);
	}
	else { /* parent */
		while((i = wait(&status)) > 0 && i != pid)
			;
		/* map cpf error returns to sysexit values */

		if (status & 0xff)
			return(EX_SOFTWARE);

		switch ((status>>8) & 0xff) {

		case 0:
			return(EX_OK);

		case 1: /* just about everything */
			return(EX_USAGE);

		case 2: /* configuration problems */
			return(EX_SOFTWARE);

		case 3: /* file permisions */
			return(EX_NOPERM);

		case 4: /* duff host */
			return(EX_NOHOST);

		case 5: /* from own child */
			return(EX_UNAVAILABLE);

		default:
			return(EX_SOFTWARE);
		}
	}
	/* NOTREACHED */
}

/*
 *  abort/interrupt routine - just cleans up and exits.
 */
abort(i)
{
	(void) unlink(mail_file);
	exit(i);
}

  
char *relay(hostentp)
struct host_entry *hostentp;
	/* Check the address for the host we are sending to and try to find
	 * a DTE which doesn't use an application relay. If we can't, return
	 * the (first - it's usually JANET) address for the site to send to.
	 */
{
	register char *p;
	register int i;
	register char *rly = NULL;

	for(i = 0; i < hostentp->n_nets; i++)
		for (p = hostentp->n_addrs[i].mail_addr; p; )

			/* AR field ? Remember the "best" in rly */
			if(*p == 'A' && p[1] == 'R')
			{	p += 2;
				while(*p == ' ') p++;
				if (!rly) rly = p;
				if(p = index(p,'\n')) *p = 0;
				p=NULL;		/* Yuck -- exit from loop */
				break;
			}
			else if (p = index(p,'\n')) p++;
			/* p is exhausted -> direct route -> use it ! */
			else return NULL;

	/* If rly is NULL, we have no mail address to send to. However
	 * we'll just ignore this possibility since cpf and friends
	 * can produce fairly detailed diags.
	 */
	return rly;
}
