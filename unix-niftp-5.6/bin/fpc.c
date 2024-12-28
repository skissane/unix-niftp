/* unix-niftp bin/fpc.c $Revision: 5.6.1.1 $ $Date: 1992/02/07 14:41:16 $ */
/*
 * $Log: fpc.c,v $
 * Revision 5.6.1.1  1992/02/07  14:41:16  pb
 * improve diagnostics, etc
 *
 * Revision 5.5  90/08/01  13:30:15  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:02:24  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:04:06  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:57:13  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:17:43  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * FTP spooler control program. This is used to control the actions
 * of the ftp spooler.
 *
 * Basic commands are:-
 *
 * start all    <qnames>
 * start queue  <qname>
 * start host   <hname>
 * stop all     <qnames>
 * stop queue   <qname>
 * stop host    <hname>
 * enable queue <qname>
 * enable host  <hname>
 * enable all   <qnames>
 * terminate all <qnames>
 * terminate queue  <qname>
 * Show queue   <qname>
 * show all     <qnames>
 * show host    <hname>
 * hosts
 * listen
 * abort spooler
 * kill spooler
 * stop listen  <listener>
 * stop listen all <listeners>
 * start listen <listener>
 * start listen all <listeners>
 * enable listen <listener>
 * enable listen all <listeners>
 * disable listen <listener>
 * setup listen <listener>
 * setup queue <qname>
 */

#include "opts.h"
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>
#include "qfiles.h"
#include "nrs.h"
#include "retry.h"

/*
 * structure used for each host, has a pointer to it for each queue
 * Information needed is basically the offset into the file of the queue
 * entry switch and the host entry switch. This need only be set up once
 * since this program will never ( knowingly change the size of the file)
 */

/* structure used to control the listenrs */

struct  listen  {
	struct  LISTEN  *li_Lp;         /* pointer into LISTEN structure */
	int     li_enable;              /* address in argconfig */
	struct  listen  *li_nli;        /* pointer to next listener */
	};


/* values used for token reading */
#undef DEBUG

#define START           1
#define STOP            2
#define ENABLE          3
#define TERMINATE       5
#define SHOW            6
#define KILL            7
#define ALL             8
#define QQUEUE          9
#define HOST            10
#define SPOOLER         11
#define LLISTEN         12
#define UNKNOWN         14
#define QUIT            15
#define COMMANDS        16
#define HELP            17
#define DISABLE         18
#define DEBUG		19
#define HOSTS           20
#define SETUP           21
#define TIMES           22
#define REREAD          23

struct  tokens  {
	char    *sstr;
	int     toke;
	} tokens[] = {
	"start",        START,
	"stop",         STOP,
	"enable",       ENABLE,
	"terminate",    TERMINATE,
	"show",         SHOW,
	"kill",         KILL,
	"all",          ALL,
	"queue",        QQUEUE,
	"host",         HOST,
	"spooler",      SPOOLER,
	"listen",       LLISTEN,
	"quit",         QUIT,
	"q",            QUIT,
	"commands",     COMMANDS,
	"disable",      DISABLE,
	"help",         COMMANDS,
	"?",            HELP,
	"h",            HELP,
	"hosts",        HOSTS,
	"setup",        SETUP,
	"reread",       REREAD,
	0,0,
	};

#define NEVER   (long)0x7FFFFFFF

struct  listen  *lstart;        /* start of list of listeners */

char    iline[256];             /* input line for interactive use */
char    *ilinep;                /* pointer used in tokenising */
char    *iname;                 /* name of object to be processed */
char    *fname;                 /* name of un recognised token */

char    *gets();
char    *malloc();
char    *strcpy();
char    *storestr();

/* used by the token routine to say that there is an item already read
 * and can be processed quickly
 */

#define H_AHEAD         1
#define Q_AHEAD         2
#define L_AHEAD         4

int     ahead;
int     sigtosend;
int     towrite;

int     interactive;    /* flag to say if interactive */
int     ftp_pid;
int     XCOUNT;
char    *Itos();

struct  bfile   Bfile;

#define ENOUGH  100

#define Q_SCAN  qp = QUEUES ; qp->Qname ; qp++
#define L_SCAN  lp = lstart ; lp ; lp = lp->li_nli
#define H_SCAN  XCOUNT = 0; Cp = dbase_get(Itos(XCOUNT)) ; XCOUNT++

main(argc,argv)
char    **argv;
{
	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise.\n");
		exit(2);
	}
	read_table(0);
	getspooler(0);
	if(--argc <= 0){
		interactive++;
		for(;;){
			printf("FTP ? ");
			fflush(stdout);
			if(gets(iline)==NULL)
				exit(0);
			process(iline);
			pushsig();
		}
	}
	for(argv++;argc;argc--,argv++){
		process(*argv);
		pushsig();              /* actually send the signals */
	}
	exit(0);
}

/* get the spoolers process id */

getspooler(x)
{
	register fd;
	fd = open(NRSdspooler,0);
	if(fd < 0){
		if(!x)
			printf("Cannot find spooler\n");
		ftp_pid = 0;
		return;
	}
	(void) read(fd,(char *)&ftp_pid, sizeof(ftp_pid));
	(void) close(fd);
}

/* routine to decode the given command */

#define queues  QUEUE

process(line)
char    *line;
{
	register t,v;
	register struct QUEUE  *qp;
	register struct listen  *lp;
	register struct host_entry *Cp;
	register char   *hp;
	struct  QUEUE  *qtoken();
	char    *htoken();
	struct  listen  *ltoken();

	ilinep = line;
	ahead = 0;
	sigtosend = 0;
	towrite=0;
	switch(t = token()){
		case STOP:
		case TERMINATE:
		case DISABLE:
			switch(token()){
				case ALL:
					for(Q_SCAN)
						stop(qp, NULL, t);
					break;
				case QQUEUE:
					qp = qtoken();
					stop(qp, NULL, t);
					break;
				case HOST:
					hp = htoken();
					stop(NULL, hp, t);
					break;
				case LLISTEN:
					if(!ahead){
						v = token();
						if(v == ALL){
							for(L_SCAN)
								stopl(lp,t);
							break;
						}
					}
					lp = ltoken();
					stopl(lp,t);
					break;
				default:
					printf("Cannot stop nothing\n");
					break;
			}
			break;
		case START:
		case ENABLE:
			switch(token()){
				case ALL:
					for(Q_SCAN)
						if (start(qp, NULL, t))
							fprintf(stderr, "	start of Q %s failed\n", qp->Qname);
					break;
				case QQUEUE:
					qp = qtoken();
					if (start(qp, NULL, t))
						fprintf(stderr, "	start of %s failed\n", qp -> Qname);
					break;
				case HOST:
					hp = htoken();
					if (!hp) fprintf(stderr, "Uknown host\n");
					else if (start(NULL, hp, t))
						fprintf(stderr, "	start of %s failed\n", hp);
					break;
				case LLISTEN:
					if(!ahead){
						if(token()==ALL){
							for(L_SCAN)
								startl(lp,t);
							break;
						}
					}
					lp = ltoken();
					startl(lp,t);
					break;
				default:
					printf("Unknown queue\n");
					break;
			}
			break;
		case SHOW:
			switch(token()){
				case NULL:
				case ALL:
					for(Q_SCAN)
						show(qp, NULL, 0);
					break;
				case QQUEUE:
					qp = qtoken();
					show(qp, NULL, 0);
					break;
				case HOSTS:
					v = token();
					for(H_SCAN)
						show(NULL,
						    Cp->host_alias, v == ALL);
					break;
				case HOST:
					hp = htoken();
					show(NULL, hp, 1);
					break;
				case LLISTEN:
					if(!ahead){
						if((v=token())==ALL||v==NULL){
							for(L_SCAN)
								showl(lp);
							break;
						}
					}
					lp = ltoken();
					showl(lp);
					break;
				default:
					printf("Unknown queue in show\n");
					break;
			}
			break;
		case HOSTS:
			v = token();
			for(H_SCAN)
				show(NULL,Cp->host_alias, v == ALL);
			break;
		case LLISTEN:
			if(!ahead){
				if((v=token())==ALL || v == NULL){
					for(L_SCAN)
						showl(lp);
					break;
				}
			}
			lp = ltoken();
			showl(lp);
			break;
		case QQUEUE:
			if(!ahead){
				if((v = token())==ALL || v == NULL){
					for(Q_SCAN)
						show(qp, NULL, 0);
					break;
				}
			}
			qp = qtoken();
			show(qp, NULL, 0);
			break;
		case KILL:
			if(interactive){
				printf("Cannot kill daemon interactively\n");
				break;
			}
			getspooler(1);
			if(!ftp_pid){
				printf("Daemon not running\n");
				break;
			}
			if(kill(ftp_pid,SIGTERM)){
				printf("Kill failed\n");
				ftp_pid = 0;
			}
			break;
		case REREAD:
			send_sig(SIGHUP);
			break;
		case QUIT:
			exit(0);
			/*NOTREACHED*/
		case COMMANDS:  /* Give a longer listing */
			printf("Commands are:-\n");
			give_help(1);
			break;
		case HELP:      /* Give list of commands */
			give_help(0);
			break;
		case NULL:    /* a single newline */
			break;
		case SETUP:
			switch(token()){
			case LLISTEN:
				lp = ltoken();
				setup( (struct queue *)NULL, lp);
				break;
			case QQUEUE:
				qp = qtoken();
				setup(qp, (struct listen *)NULL);
				break;
			default:
				printf("Cannot set up nothing\n");
				break;
			}
			break;
		case TIMES:
			settimes();
			break;
		default:
			printf("Unknown command\n");
			break;
	}
}

/* routine to decode a token */

token()
{
	register char   *cp,*ocp;
	register struct tokens  *tp;
	register struct QUEUE  *qp;
	register struct listen  *lp;

	/* get rid of white space */
	for(cp = ilinep;*cp == ' ' || *cp == '\t' ; cp++);

	if(*cp == 0)
	{	fprintf(stderr, "cp is null, so token returns null\n");
		return(NULL);
	}

	ocp = cp;
	/* Now find the length of the word */
	while( *cp && *cp != ' ' && *cp != '\t' )
		cp++;

	if(*cp)
		*cp++ = 0;
	ilinep = cp;    /* store back for later use */

	/* now look it up in the token table */
	for(tp = tokens ; tp->sstr ; tp++)
		if(strcmp(tp->sstr,ocp)==0)
			return(tp->toke);

	/* not found in the tokens look them up in the queues */
	for(Q_SCAN)
		if(strcmp(qp->Qname, ocp) == 0){
			iname = ocp;
			ahead = Q_AHEAD;
			return(QQUEUE);
		}

	/* lookup in listeners */

	for(L_SCAN)
		if(strcmp(lp->li_Lp->Lname,ocp)==0){
			iname = ocp;
			ahead = L_AHEAD;
			return(LLISTEN);
		}

	/* now look up in hosts */

	if(dbase_find(ocp, (char *) 0, 0) != NULL){     /* valid */
		iname = ocp;
		ahead = H_AHEAD;
		return(HOST);
	}

	return(UNKNOWN);
}


struct  QUEUE *
qtoken()
{
	register struct QUEUE *qp;

	if(!ahead)
		if(token() != QQUEUE){
			printf("Unknown queue\n");
			return(NULL);
		}
	/* must have ahead set now */

	if( ahead != Q_AHEAD ){
		printf("Unknown queue\n");
		return(NULL);
	}

	ahead = 0;
	for(Q_SCAN)
		if(strcmp(qp->Qname, iname)==0)
			return(qp);
	return(NULL);
}

/*
 * routine to return a pointer to a host
 */

char    *
htoken()
{
	struct  host_entry      *hp;

	if(!ahead)
		if(token() != HOST){
			printf("Unknown host %s\n", fname);
			return(NULL);
		}
	/* must have ahead set now */

	if( ahead != H_AHEAD ){
		printf("Unknown host ...\n");
		return(NULL);
	}

	ahead = 0;
	/* now look up in hosts */

	if(hp = dbase_find(iname, (char *) 0, 0))
		return(hp->host_alias);
	fprintf(stderr, "Unknown host %s\n", iname);
	return(NULL);
}

struct  listen *
ltoken()
{
	register struct listen *lp;
	if(!ahead && token() != LLISTEN){
		printf("Unknown listener\n");
		return(NULL);
	}

	/* must have ahead set now */

	if( ahead != L_AHEAD ){
		printf("Unknown listener\n");
		return(NULL);
	}

	/* Now look it up */

	ahead = 0;
	for(L_SCAN)
		if(strcmp(lp->li_Lp->Lname, iname) == 0)
			return(lp);
	return(NULL);
}

/* here are some of the basic commands */

show(qp, hp, f)
register struct QUEUE *qp;
char   *hp;
{
	register struct host_entry      *Cp;
	struct  NETWORK *np;
	int     i;
	char    *qn;
	char    *Sname();

/* deal with queues first off */

	if(qp){
		printf("%s   ",qp->Qname);
		if(isqenable(qp))
			printf("(Enabled) ");
		else
			printf("(Disabled)");
		qn = NULL;
		for(np = NETWORKS ; np->Nname ; np++)
			if(strcmp(qp->Qname, np->Nqueue) == 0 ){
				qn = np->Nname;
				break;
			}

		for(H_SCAN){
			if(qn == NULL){
				/* if(!Cp->n_localhost) */
					continue;
			}
			else {
				for(i = 0; i < Cp->n_nets ; i++){
					if(Cp->n_addrs[i].net_name == NULL)
						continue;
					if(strcmp(Cp->n_addrs[i].net_name,
								   qn) == 0)
						break;
				}
				if(i >= Cp->n_nets)
					continue;
				if(Cp->n_addrs[i].ftp_addr == NULL &&
					  Cp->n_addrs[i].mail_addr == NULL)
					continue;
			}
			printf("\t%s ", Sname(Cp->host_alias));
			if(Cp->n_disabled)
				printf("(off) ");
		}
		printf("\n");
		return;
	}

/* now deal with hosts */

	if(!hp)
		return;

	printhost(hp, f);
}

/* routine to print out the state of a given host */
/* f unused ?? */

/* ARGSUSED */
printhost(hp, f)
register char   *hp;
{
	char    xbuf[40];
	struct  host_entry      *Cp;

	Cp = dbase_find(hp, (char *) 0, 0);     /* this should always work */
	if (!Cp) sprintf(xbuf, "[no host %s] ", hp);
	else if(Cp->n_disabled)
		sprintf(xbuf, "%s (off)\t", Cp->host_alias);
	else
		sprintf(xbuf, "%s\t", Cp->host_alias);
	/*
	 * should look through the Bfiles...
	 */
}

/* routine to stop transfers */

stop(qp,hp,v)
register struct QUEUE *qp;
register char   *hp;
{
	/* wants a queue stopped */
	if(qp){
		qdisable(qp, 1);
		if(v == DISABLE)
			return;
		else if(v == STOP)
			send_sig(SIGQUIT);
		else if(v == TERMINATE)
			send_sig(SIGTRAP);
		return;
	}
	if(!hp)
		return;

	/* wants a host stopping */
	if (hdisable(hp, 1))
		fprintf(stderr, "	failed to disable %s\n", hp);
	if(v == DISABLE)
		return;
	else if(v == STOP)
		send_sig(SIGQUIT);
	else if(v == TERMINATE)
		send_sig(SIGTRAP);
}

/* routine to enable transfers */

start(qp,hp,v)
register struct QUEUE *qp;
register char   *hp;
{
	/* wants a queue starting */

	if(qp){
		qdisable(qp, 0);
		if(v == ENABLE)
			send_sig(SIGQUIT);
		else
			send_sig(SIGPIPE);
		return 0;
	}

	if(!hp)
		return 1;

	/* wants a host starting */
	if (hdisable(hp, 0))
		fprintf(stderr, "	failed to enable %s\n", hp);
	if(v != ENABLE) send_sig(SIGPIPE);
	return 0;
}

/* routine to queue up a signal to be sent to then spooler */

send_sig(sig)
{
	if(sigtosend && sig != sigtosend)
		pushsig();
	sigtosend = sig;
}

/* routine to send a signal to the spooler */

pushsig()
{
	if(!sigtosend)
		return;
	getspooler(1);
	if(!ftp_pid){
		printf("Spooler not running\n");
		return;
	}
	if(kill(ftp_pid, sigtosend))
		printf("Kill failed\n");
	(void) sleep(1);
}

/* a routine to print out some help */

give_help(more)
{
	int     Fh,n;
	char	file[ENOUGH];
	char	buff[BUFSIZ];
	char    *f;

	f = (more ? HELPFILE : HOWFILE);
	if (f[0] != '/') {
		sprintf(file, "%s/%s", BINDIR, f);
		f = file;
	}

	if( (Fh = open(f, 0)) < 0 ){
		printf("Cannot open helpfile (%s)\n",f);
		return;
	}

	while((n = read(Fh, buff, BUFSIZ)) > 0)
		(void) write(1, buff, n);
	(void) close(Fh);
}

/* Code to deal with listeners */

/*
 * generate the name of a listen control file
 */

char    *
tolname(lp)
struct  listen  *lp;
{
	static  char    xbuff[100];
	struct  LISTEN  *Lp = lp->li_Lp;

	if(*Lp->Lname != '/')
		sprintf(xbuff, "%s/.L.%s", NRSdqueue, Lp->Lname);
	else
		(void) strcpy(xbuff, Lp->Lname);
	return(xbuff);
}

/* start up a listener */

startl(lp,t)
register struct listen *lp;
{
	if(lp == NULL)
		return;
	if(close(creat(tolname(lp), 0)) < 0)
		fprintf(stderr, "Enable creat failed\n");

	lp->li_enable = 1;
	if(t == ENABLE)
		send_sig(SIGFPE);
	else
		send_sig(SIGSYS);
}

/* stop a listener */

stopl(lp,t)
register struct listen  *lp;
{
	if(lp == NULL)
		return;
	(void) unlink(tolname(lp));
	lp->li_enable = 0;

	if(t == DISABLE)
		send_sig(SIGFPE);
	else
		send_sig(SIGIOT);
}

/* print out the status of a listener */

showl(lp)
register struct listen *lp;
{
	struct  LISTEN  *Lp;

	if(lp == NULL)
		return;
	Lp = lp->li_Lp;
	printf("%s   ",Lp->Lname);
	if(lp->li_enable)
		printf("(Enabled) ");
	else
		printf("(Disabled)");
	if(Lp->Lprog)
		printf("Daemon %s  ", Lp->Lprog);
	if(Lp->Llevel)
		printf("logging level %d  ",Lp->Llevel);
	if(Lp->Llogfile)
		printf("Logging file %s",Lp->Llogfile);
	printf("\n");
}

/* setup the various bits of the argconfig file */


setup(qp,lp)
register struct QUEUE  *qp;
register struct listen  *lp;
{
	char    *storestr();

	if(qp){
		printf("Edit the configure file\n");
		send_sig(SIGFPE);
		return;
	}
	if(!lp)
		return;
	printf("Edit the configure file\n");
	send_sig(SIGFPE);
}

/* read in the Q process configuration tables */
/* args unused */

/* ARGSUSED */
read_table(sig)
{
	register struct listen *lp;
	struct  LISTEN  *Lp;
	char    *storestr();

	/* get the Q processes */

	for(Lp = LISTENS ; Lp->Lname ; Lp++){
		/* look for it in listen Q */
		for(L_SCAN)
			if(strcmp(lp->li_Lp->Lname, Lp->Lname) == 0)
				break;
		if(!lp){        /* create a new listener slot */
			lp = (struct listen *) malloc( sizeof(* lp) );
			lp->li_nli = lstart;
			lp->li_Lp = Lp;
			lstart = lp;
		}
		/* check to see it is enabled */
		lp->li_enable = (access(tolname(lp), 0) == 0);
	}
}

char *
storestr(xstr)
char *xstr;
{
	char    *l,*strcpy();
	if((l = malloc(strlen(xstr) + 1)) == NULL){
		printf("Out of core\n");
		exit(2);
	}
	return(strcpy(l,xstr));
}

/* routine to write the argconfig file */

writeargs()
{
}

/*
 * routine to look at the start and stop times of the queues and listeners
 * and to start/ stop them as appropriate
 */

settimes()
{
}

char    *
toqname(qp)
struct  QUEUE   *qp;
{
	static  char    xbuff[100];
	char    *q;

	if( (q = qp->Qdir) == NULL)
		q = qp->Qname;

	if(*q != '/')
		sprintf(xbuff, "%s/%s", NRSdqueue, q);
	else
		(void) strcpy(xbuff, q);
	return(xbuff);
}

isqenable(qp)
struct  QUEUE  *qp;
{
	struct  stat    statbuf;

	if(stat(toqname(qp), &statbuf) < 0)
		return(0);
	if(statbuf.st_mode & S_IEXEC)
		return(1);
	return(0);
}

qdisable(qp, disable)
struct  QUEUE  *qp;
{
	struct  stat    statbuf;
	char    *q = toqname(qp);

	if(stat(q, &statbuf) < 0){
		printf("Cannot access queue\n");
		return;
	}
	if(disable)
		statbuf.st_mode &= ~S_IEXEC;
	else
		statbuf.st_mode |= S_IEXEC;
	if(chmod(q, statbuf.st_mode & 0777) < 0)
		printf("Cannot change queue status\n");
}

hdisable(hp, disable)
char    *hp;
{
	int     i, fd;
	struct  host_entry      *Cp;
	register struct backoff *bp, *xb;
	struct  NETWORK *np;
	struct  QUEUE   *qp;
	char    *xp;
	char    xbuf[ENOUGH];


	if( (Cp = dbase_find(hp, (char *) 0, 0)) == NULL)
		return 1;

#ifdef  notdef
	if(Cp->n_localhost){
		i = MAXNETS;
		for(xp = NULL, np = NETWORKS ; np->Nname ; np++)
			if(*np->Nname == 0){
				xp = np->Nqueue;
				break;
			}
		if(xp == NULL){
			fprintf(stderr, "No local net!!\n");
			return;
		}
		goto gotlocal;
	}
#endif
	for(i = 0 ; i < MAXNETS ; i++){
		if(Cp->n_addrs[i].net_name == NULL)
			continue;
		for(np = NETWORKS ; np->Nname ; np++)
			if(strcmp(Cp->n_addrs[i].net_name, np->Nname) == 0)
				break;
		if(np->Nname == NULL)
			continue;
#ifdef  notdef
	gotlocal:;
		if(xp == NULL){
			fprintf(stderr, "No net!!\n");
			return 1;
		}
#endif
		/*
		 * now find out which queue we are currently playing with
		 */
		for(qp = QUEUES ; qp->Qname ; qp++)
			if(strcmp(np->Nqueue, qp->Qname) == 0)
				break;
		if(qp->Qname == NULL){
			printf("Unknown queue for network %s\n", np->Nname);
			continue;
		}
		if( (xp = qp->Qdir) == NULL)
			xp = qp->Qname;
		if(*xp != '/')
			sprintf(xbuf, "%s/%s/%s", NRSdqueue, xp, BCONTROL);
		else
			sprintf(xbuf, "%s/%s", xp, BCONTROL);
		fd = open(xbuf, 2);
		if(fd < 0){
			printf("Cannot open %s\n", xbuf);
			break;
		}
		lockfile(fd);
		if(read(fd, (char *)&Bfile, sizeof(Bfile))!=sizeof(Bfile)){
			printf("Read failure on Bfile\n");
			(void) close(fd);
			break;
		}
		(void) lseek(fd, (long)0, 0);
		xb = NULL;
		for(bp = Bfile.b_hosts ; bp < &Bfile.b_hosts[MAXBHOSTS];bp++)
			if(bp->b_host[0] == 0){
				if(xb == NULL)
					xb = bp;
				continue;
			}
		else if(strncmp(bp->b_host, Cp->host_alias, BFSIZE-1) == 0)
				break;
		if(bp >= &Bfile.b_hosts[MAXBHOSTS]){
			/* not already there */
			if(!disable){   /* starting it up. ignore */
				fprintf(stderr, "	%s already enabled on %s\n",
					Cp->host_alias, xp);
				(void) close(fd);
				continue;
			}
			if(xb == NULL){
				printf("No space in Bfile\n");
				(void) close(fd);
				continue;
			}
			(void)strncpy(xb->b_host, Cp->host_alias, BFSIZE-1);
			xb->b_hc.h_nextattempt = NEVER;
			if(xb-Bfile.b_hosts >= Bfile.b_nhosts)
				Bfile.b_nhosts = xb - Bfile.b_hosts;
		}
		else {
			/* matched the host */
			if(disable)
				bp->b_hc.h_nextattempt = NEVER;
			else
				(void)time(&bp->b_hc.h_nextattempt);
			printf("	%s %sabled on %s\n", Cp->host_alias,
				(disable) ? "dis" : "en", xp);
		}
		if(write(fd, (char *)&Bfile, sizeof(Bfile)) != sizeof(Bfile))
			printf("Write failed on Bfile\n");
		(void) close(fd);
	}
	return 0;
}

char    *
Itos(numb)
register numb;
{
	static  char    x[10];
	register char   *p;

	p = &x[9];
	*p-- = 0;
	do {
		*p-- = numb % 10 + '0';
	}while(numb /= 10);
	*p-- = '#';
	*p = '#';
	return(p);
}

char    *
Sname(name)
char    *name;
{
	register char   **xp, *p, *q;

	for(xp = NRSdomains ; (p = *xp) ; xp++){
		for(q = name ; *p && *p++ == *q++ ;);
		if(!*p)
			return(q);
	}
	return(name);
}
