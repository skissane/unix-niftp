/* unix-niftp bin/ftpspool.c $Revision: 5.6 $ $Date: 1991/06/07 16:59:30 $ */
/*
 * the main ftp spooler
 *      job is to control all streams
 *
 *      Basic Job is to start up all daemons at reboot time
 *              Then wait for signals to arrive from cpf's
 *              On arrival start up daemon (if there is one )
 *      Since this knows which daemons are running then this is
 *              No problem.
 *  Was totally interupt driven but has been converted to work with queues of
 * Signals ( Otherwise the it gets totally screwed. )
 *
 * has been modified to work on 4.2 define _42 to get the mods
 * $Log: ftpspool.c,v $
 * Revision 5.6  1991/06/07  16:59:30  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:30:35  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:35:25  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:10:54  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:30:35  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0  88/01/28  06:21:58  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0  87/03/23  03:18:47  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
/*
 * structure used for each host, has a pointer to it for each queue
 * Information needed is basically the offset into the file of the queue
 * entry switch and the host entry switch. This need only be set up once
 * since this program will never ( knowingly change the size of the file)
 */
#include "opts.h"
#include "nrs.h"
#include "retry.h"
#include <sys/stat.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <setjmp.h>
#include <errno.h>
#include "files.h"

/* Sigh .... HACK HACK HACK ....
 * We need some way to decide if to include fcntl in order to force APPEND
 * mode, rather than the usual function of file locking.
 *
 * So, on SUNOS 3.5 (at least) we DO want fcntl.h
 */
#if	((defined(F_APPEND) || defined(SIGWINCH)) && !defined(F_GETFL)) || defined(FCNTL)
#include <fcntl.h>
#endif

#define NEVER   (long)0x7FFFFFFF        /* a long time */

/* this structure is used to store the head of each queue */

#define Q_ENABLE        2               /* Q is enabled */
#define Q_TIMEOUT       4               /* Q is doing a timeout */
#define Q_SCHEDULE	8		/* Q is possibly ready to start another */
#define RESCHED_TIME	60

/* syslog levels - maybe move to syslog one day? */
#define	LOG_ALERT	1	/* alert -- send to all users */
#define	LOG_SALERT	2	/* subalert -- send to special users */
#define	LOG_EMERG	3	/* emergency conditions */
#define	LOG_ERROR	4	/* error */
#define	LOG_ERR		4	/* synonym of LOG_ERROR */
#define	LOG_CRIT	5	/* critical information */
#define	LOG_WARNING	6	/* warning */
#define	LOG_NOTICE	7	/* important information */
#define	LOG_INFO	8	/* informational message */
#define	LOG_DEBUG	9	/* debug level info */

#define	MAXSQ	20			/* max simulataneuos procs -tailor! */


typedef struct  queues  {
	char	*q_name;
	struct  QUEUE   *q_Qp;          /* pointer to QUEUE structure */
	char    *q_rfile;               /* name of retry file */
	int     q_state;                /* enable this queue */
	struct {
		int     q_pid;		/* process id of daemon */
		char	*q_name;	/* particular queue name */
		struct QUEUE *q_Qp;	/* pointer to Queue structure */
	} q_proc[MAXSQ];
	long    q_mtime;                /* time of last daemon start */
	long    q_stime;                /* time to sleep till */
	int	q_curno;		/* current no of running procs */
	int	q_maxno;		/* max allowed on this queue */
	struct  queues  *nextq;         /* pointer to next queue */
} Queues;
#define	NULLQP	((Queues *)0)

Queues	*getqbypid ();
Queues	*getqbyname ();

/* structure used to control listeners. simple */

#define L_ENABLE        1       /* listener is enabled */
#define L_TOSTOP        2       /* listener is to stop */
#define L_LOOKED        4       /* listener has already been processed */

struct  listen  {
	struct  LISTEN  *li_Lp;         /* pointer into LISTEN structs */
	int     li_pid;                 /* listener process id */
	struct  listen *li_nli;         /* the next listener in the chain */
	int     li_state;               /* various state info */
	};

struct  queues  *qstart;        /* start of list of queues */
struct  listen  *lstart;        /* start of list of listeners */

char    *malloc();
char    *strcpy();
char    *storestr();
extern	errno;

struct  bfile   Bfile;          /* the backoff file area */
int     b_fd;
struct  backoff *Lastbp;        /* pointer to last backoff entry */

#define Q_SCAN  qp = qstart ; qp ; qp = qp->nextq
#define L_SCAN  lp = lstart ; lp ; lp = lp->li_nli
#define B_SCAN  bp = Bfile.b_hosts ; bp < Lastbp ; bp++


#define	SIGNONE	0		/* No stacked up signals */

int     sigsused[] = {            /* the signals used */
	SIGINT,                         /* sent by cpf */
	SIGHUP,                         /* kill system slowly */
	SIGTERM,                        /* kill system quickly */
	SIGQUIT,                        /* reload table don't re-exec */
	SIGTRAP,                        /* reload table kill ex daemons */
	SIGPIPE,                        /* reload start stopped daemons */
	SIGALRM,                        /* Host gone away restart */
	SIGFPE,                         /* re-read argument table don't exec*/
	SIGSYS,                         /* start new Q's */
	SIGIOT,                         /* stop ex Q's */
	SIGEMT,                         /* Used to submit a jtmp job */
	SIGTTIN,                        /* play with debug level */
#ifdef SIGCHLD
	SIGCHLD,			/* just for fun ... */
#endif
	0,
	};

#ifdef JTMP
int	jtmp_pid;		/* pid of the jtmp daemon */
#endif

int     my_pid;                 /* the spoolers pid */

int     closedown;              /* close down the system */

#define CSIZE   200             /* queue of signal requests */

char    sigq[CSIZE];
char    *qhead = sigq;
char    *qtail = sigq;
char    qfull;

char    *logf;                  /* name of log file. NULL if not got one */

#ifdef  _42
jmp_buf leaver;
int     insleep;
#define LEFT    2
#endif

static int	debug = 0;
static int	no_debug = 0;
static int	high_debug = 4;
static int	user_debug = 0;
static char	*myname = "ftpspool";
static int	reopen_interval = (60 * 60);

main(argc,argv)
char    **argv;
{
	int     fd,sig,pid,status;
	register struct queues  *qp;
	register struct listen  *lp;
	register struct QUEUE   *Qp;
	struct  stat    statbuf;
	register int i;
	extern int optind;
	extern char *optarg;
	int	opt;

	myname = argv[0];
	while ((opt = getopt(argc, argv, "d")) != EOF) switch (opt) {
	case 'd':	debug++;	break;
	default:	fprintf(stderr, "Usage: %s [-d] [logfile]\n", myname);
	}

	argc -= optind;
	argv += optind;

	user_debug = debug;
	if (reopen_interval == (60 * 60) && debug > 1) reopen_interval = 5*60;

	catchsigs();    /* start of critical region */

	if(argc > 0)	logf = argv[0];

	init();         /* set up files and put daemon in background */

	logger(LOG_INFO, "%s started\n", myname);
	/* disable all other processes calling me by deleteing the infofile */

	(void) umask(0);
	(void) unlink(NRSdspooler);

	gettable(0);
	read_table(0);

	/* then start all daemons */
	/* do this by searching the directory with all the queues in */

	startdaemons();

#ifdef JTMP
	startjtmp();
#endif JTMP

	/* now enable other processes to kill me */

	my_pid = getpid();

	fd = creat(NRSdspooler,0444);
	if(fd < 0){
		logger(LOG_ERR, "ftpspool: Cannot create %s\n",NRSdspooler);
		exit(5);
	}
	(void) write(fd,(char *)&my_pid,sizeof(my_pid));
	(void) close(fd);

	setsigs();      /* end critical region */

	/* now wait for things to die */
	/* and restart if needed */

	for(;;){
		int example_child;	/* this is A child we are awaiting */

		if (qtail == qhead)
			sig = SIGNONE;
		else
		{	if (debug > 2) logger(LOG_DEBUG, "await\n");
			sig = getsig();
			if (debug > 1) logger(LOG_DEBUG, "Signal %d\n", sig);
		}

		switch(sig){
		case SIGNONE:
			for(Q_SCAN) {     /* find a running process */
				for (i = 0; i < qp -> q_maxno; i++)
					if(qp->q_proc[i].q_pid)
						break;
				if (i < qp -> q_maxno) break;
			}
			if(qp)          /* got one */
			{	if (debug > 3) logger(LOG_DEBUG,
					"found Q %d\n", qp->q_proc[i].q_pid);
				example_child = qp->q_proc[i].q_pid;
				break;
			}

			for(L_SCAN)     /* look for listeners */
				if(lp->li_pid)
					break;
			if(lp)          /* found one */
			{	if (debug > 3) logger(LOG_DEBUG,
					"found L %d\n", lp->li_pid);
				example_child = lp->li_pid;
				break;
			}

#ifdef JTMP
			if(jtmp_pid)	/* got the jtmp process */
			{	if (debug > 3) logger(LOG_DEBUG,
					"found J %d\n", jtmp_pid);
				example_child = jtmp_pid;
				break;
			}
#endif JTMP
			if(closedown){ /* time to die */
				(void) unlink(NRSdspooler);
				logger(LOG_INFO, "Finished\n");
				exit(0);
			}
			/* nothing to do */
#ifdef  _42
			/* set a return point in 4.2 */
			if(setjmp(leaver) != LEFT){
				insleep = 1;
				if (debug > 1) logger(LOG_DEBUG, "pause\n");
				pause();
				insleep = 0;
			}
#else
			if (debug > 1) logger(LOG_DEBUG, "pause\n");
			pause();
#endif
			continue;
		case SIGINT:
			rescan();
			continue;
		case SIGHUP:
			gettable(1);
			continue;
		case SIGTERM:
			logger(LOG_INFO, "Got a sig term\n");
			sigterm();
			continue;
		case SIGQUIT:
		case SIGTRAP:
		case SIGPIPE:
			sigscan(sig);
			continue;
		case SIGALRM:
			setalarm();
			continue;
		case SIGFPE:            /* deal with listeners */
		case SIGIOT:
		case SIGSYS:
			read_table(sig);
			continue;
		case SIGTTIN:
			set_debug();
			continue;
		case SIGEMT:            /* deal with jtmp process */
					/* only if jtmp not running */
#ifdef JTMP
			if(!jtmp_pid)
				startjtmp();
#endif JTMP				/* otherwise ignore */
			continue;
		}

#ifdef  _42
		if(setjmp(leaver) == LEFT)
			continue;
		insleep = 1;
		if (debug > 2) logger(LOG_DEBUG, "wait\n");
		pid = wait(&status);
		insleep = 0;
#else
		if (debug > 2) logger(LOG_DEBUG, "wait\n");
		pid = wait(&status);
#endif
		if(pid == -1)                   /* no children */
		{	if (errno == ECHILD)
			{	logger(LOG_ERR,
					"No child, so %d must be spurious\n",
					example_child);
				pid = example_child;
			}
			else
			{	logger(LOG_ERR, "Wait failed %d\n", errno);
				sleep(20);
				continue;               /* or signal */
			}
		}

		/* get child - send him away */
		/* look him up in queue */
		logger(LOG_DEBUG, "Pid %d: %x\n", pid, status);
		qp = getqbypid(pid);

		if(!qp && listenp(pid,status))  /* it's a listener */
			continue;
#ifdef JTMP
		if(!qp && jtmp_pid && pid == jtmp_pid){
			jtmpp(pid, status);
			continue;
		}
#endif JTMP
		if(!qp){        /* should never happen */
			logger(LOG_WARNING, "Unknown child %d\n",pid);
			continue;
		}

		zapqpid (qp, pid);	/* clean up the evidence */

		/*
		 * Now deal with hosts on this queue
		 */

		if(!(qp->q_state & Q_ENABLE) || closedown){
			qp->q_state =0;
			continue;
		}
		Qp = qp->q_Qp;
		cleanbfile(qp);         /* clean out the junk from the Bfile*/

		if(status & 0377) {     /* system failure restart */
			logger(LOG_INFO, "Daemon %s (%s) died with %d\n",
			       (Qp->Qprog ? Qp->Qprog : PPROC),
			       Qp->Qname, status);
			restartd(qp); 	/* start daemon */
			continue;
		}

		switch(status){
	case    0:              /* He has died cos there is nothing to do */
			logger (LOG_DEBUG,
				qp -> q_curno ?	"%s quiescent (+%d)\n" :
					"%s quiescent\n",
				qp->q_name, qp -> q_curno);
			(void) stat(qp->q_name, &statbuf);
			qp->q_mtime = statbuf.st_mtime;
			qp -> q_state &= ~Q_SCHEDULE; /* don't start any more*/
			sett(qp);
			setalarm ();
			break;

	case    ((-1)<<8)&0xFF00: /* he has died cos couldn't exec daemon */
			qp->q_state &= ~Q_ENABLE;
			logger (LOG_WARNING, "Disabling Queue %s\n",
				qp -> q_name);
			break;
	case    24<<8:          /* Host has died try in a few mins */
			qp -> q_state &= ~Q_SCHEDULE; /* no more in parallel */
			sett(qp);
			setalarm();
			qp -> q_state |= Q_SCHEDULE; /* ... until we restart */
			break;
		}
	}
}

/*
 * the initialisation phase. close all files and put daemon in
 * the background.
 */

init()
{
	int     i;

	if(nrs_init()){
		logger(LOG_ERR, "Cannot initialise\n");
		exit(1);
	}

	if (!debug) {
		while((i = fork()) < 0);
		if(i)		/* parent dies */
			_exit(0);
		/*
		 * daemon is now the child process. Process is now in background
		 */
		for(i = 0 ; i < 20 ; i++) /* close all files */
			(void) close(i);
		(void) open("/dev/null",2); /* open 0,1,2 */
		(void) dup(0);
		(void) dup(1);
#ifdef  TIOCNOTTY
		i = open("/dev/tty", 2);
		(void) ioctl(i, TIOCNOTTY, 0);
		(void) close(i);
#endif
	}

#ifdef	MAXFILESIZE
	ulimit(2, MAXFILESIZE/512);
#endif	MAXFILESIZE
}
/* routine to find out when the queue should be restarted */

sett(qp)
register struct queues *qp;
{
	register struct backoff *bp;
	long    ttime = 0;
	long    xtime;

	(void) time(&xtime);
	if(readbfile(qp) >= 0){
		for(B_SCAN){
			if(!*bp->b_host || !bp->b_hc.h_nextattempt)
				continue;
			if(!ttime)
				ttime = bp->b_hc.h_nextattempt;
			else if(bp->b_hc.h_nextattempt < ttime)
				ttime = bp->b_hc.h_nextattempt;
		}
	}
	if(!ttime || ttime < xtime) {
/*
 * clever bit - well hack really
 * If there is a process running and there is room for another then
 * we try firing another one of in RESCHED_TIME seconds or so
 */
		if ( (qp ->q_state & Q_SCHEDULE) &&
		    qp -> q_curno && qp->q_curno < qp->q_maxno) {
			ttime = xtime + RESCHED_TIME;
			logger (LOG_DEBUG, "Schedule another %s\n",
				qp -> q_name);
		}
		else
			ttime = xtime + 20*60;
	}
	qp->q_stime = ttime;
	qp->q_state |= Q_TIMEOUT;
}

/* routine to start all daemons at start up */

startdaemons()
{
	register struct queues  *qp;

	for(Q_SCAN){
		writebfile(qp, 1);
		if(isqenable(qp))
			qp->q_state |= Q_ENABLE;
		/* now enable the queue */

		if(qp->q_state & Q_ENABLE) {
			qp -> q_state |= Q_SCHEDULE;
			restartd(qp);
		}
	}
}


/* Called when someone has changed a directory ( cpf ) */

rescan()
{
	register struct queues  *qp;
	struct  stat    statbuf;

	for(Q_SCAN){
		if(qp->q_name && qp->q_curno < qp->q_maxno){
			if(stat(qp->q_name,&statbuf) < 0)
				continue;
			if(statbuf.st_mtime > qp->q_mtime){
					/* right start this daemon */
				qp->q_mtime = statbuf.st_mtime;
				qp->q_state &= ~Q_TIMEOUT;
				qp->q_state |= Q_SCHEDULE;
				restartd(qp);
			}
		}
	}
}

/* routine to actually fire up the daemon */

restartd(qp)
register struct queues  *qp;
{
	register struct QUEUE   *Qp;
	register pid;
	register int i;

	if(closedown) {
		qp->q_state = 0;
		return;
	}
	if(! (qp->q_state & Q_ENABLE) )
		return;
	if (qp -> q_curno >= qp -> q_maxno)
		return;

	for(i = 0; i < qp->q_maxno; i++)
		if (qp->q_proc[i].q_pid == 0) {
			break;
		}
	while( (pid = fork()) < 0);
	if(pid == 0){
		char    xb[100];

		Qp = qp->q_proc[i].q_Qp;
		if(Qp->Qprog == NULL)
			Qp->Qprog = PPROC;
		if(*Qp->Qprog != '/')
			sprintf(xb, "%s/%s", BINDIR, Qp->Qprog);
		else
			(void) strcpy(xb, Qp->Qprog);

		logger(LOG_DEBUG, "exec %s %s %s\n", xb, Qp->Qprog, Qp->Qname);
		execl(xb, Qp->Qprog, Qp->Qname, 0);
		logger(LOG_ERR, "exec of %s failed\n", xb);
		_exit(-1);
		/*NOTREACHED*/
	}
/*
 * OK - here we mark the pid, then force a rescan of the queue struct
 * this will then cause a timeout at some point if there is room for
 * another process.
 */
	qp->q_proc[i].q_pid = pid;
	qp -> q_curno ++;
	logger(LOG_DEBUG, "queue %s, process %d started (now %d)\n",
	       qp -> q_name, pid, qp -> q_curno);
	sett (qp);
	setalarm ();
}

int     termsig;        /* set if got SIGTERM */

catchsigs()
{
	register int    *sp;

	if(termsig)
		return;
	for(sp = sigsused ; *sp; sp++)
		(void) signal(*sp, SIG_IGN);
}

setsigs()
{
	register int    *sp;
	SIGRET     sigcall();

	if(termsig)
		return;
	for(sp = sigsused ; *sp; sp++)
		(void) signal(*sp, sigcall);
}

SIGRET sigcall(sig)
{
	register int    *sp;
	(void) signal(sig, SIG_IGN);
	if(sig != SIGINT)
		(void) signal(SIGINT, SIG_IGN);
	if(qfull)               /* HELP !! */
		goto out;
	if(termsig)             /* should never occur */
		return;
	*qtail++ = sig;
	if(qtail >= &sigq[CSIZE])
		qtail = sigq;
	if(qtail == qhead)
		qfull++;
out:
	if(sig==SIGTERM){
		termsig++;
		for(sp = sigsused ; *sp; sp++)
			(void) signal(*sp, SIG_IGN);
	}
	else {
		(void) signal(sig, sigcall);
		if(sig != SIGINT)
			(void) signal(SIGINT, sigcall);
	}
#ifdef  _42
	/*
	 * cause an abnormal return when in a pause or a wait
	 * 4.2 is such a pain
	 */
	if(insleep){
		insleep = 0;
		longjmp(leaver,LEFT);
	}
#endif
}

getsig()
{
	register retv;

	(void) signal(SIGINT, SIG_IGN);
	if(qtail == qhead)
		retv = SIGNONE;
	else {
		retv = *qhead++ & 0377;
		if(qhead >= &sigq[CSIZE])
			qhead = sigq;
	}
	qfull = 0;
	(void) signal(SIGINT, sigcall);
	return(retv);
}

/* rescan table and find out when to next try a host */

setalarm()
{
	register struct queues  *qp;
	long    ttime;
	long    togo = 0;

	if(closedown)
		return;
	(void) time(&ttime);
	(void) alarm(60);       /* give us some grace */

	for(Q_SCAN){
		if(!(qp->q_state & Q_TIMEOUT) || qp->q_curno >= qp->q_maxno)
			continue;
		if(!togo)
			togo = qp->q_stime;
		if(qp->q_stime < togo)
			togo = qp->q_stime;
	}

	if(!togo){      /* Nothing waiting */
		(void) alarm(30*60);    /* must always check */
		return;
	}

	if(togo <= ttime){
		/* should start again now */
		for(Q_SCAN)
			if(ttime >= qp->q_stime){
				qp->q_state &= ~Q_TIMEOUT;
				restartd(qp);
			}
	}
	else
		(void)alarm(togo - ttime);
}


/* to stop the processes quickly */

sigterm()
{
	register struct listen  *lp;
	register struct queues  *qp;
	register int i;

	closedown = 1;
	for(Q_SCAN)             /* kill of P processes */
		for (i = 0; i < qp->q_maxno; i++)
			if(qp->q_proc[i].q_pid){
				qp->q_state = 0;
				logger(LOG_DEBUG, "Killing %d\n",
				       qp->q_proc[i].q_pid);
				(void) kill(qp->q_proc[i].q_pid, SIGHUP);
			}

	for(L_SCAN)             /* kill off Q processes */
		if(lp->li_pid){
			lp->li_state = 0;
			logger(LOG_DEBUG, "Killing %d\n", lp->li_pid);
			(void) kill(lp->li_pid, SIGHUP);
		}

#ifdef JTMP
	if(jtmp_pid)
		(void) kill(jtmp_pid, SIGHUP);
#endif JTMP
}

/* re-load table, */

sigscan(sig)
int     sig;
{
	register struct queues  *qp;
	register struct backoff *bp;
	register enabled,qenable;
	long    n_time;
	register int i;

	(void) time(&n_time);
	for(Q_SCAN){
		enabled = 0;
		qenable = 0;
		if(closedown && sig != SIGTRAP){
			qp->q_state = 0;
			continue;
		}
		if(isqenable(qp))
			qenable++;
		if(readbfile(qp) < 0)
			goto out1;
		for(B_SCAN){
			if(*bp->b_host && bp->b_hc.h_nextattempt != NEVER)
				enabled++;
		}
	out1:;
		if(qenable)
			qp->q_state |= Q_ENABLE;
		else
			qp->q_state &= ~Q_ENABLE;

		if(sig == SIGQUIT)
			continue;
		if(sig == SIGTRAP){
			if(enabled && qenable && !closedown)
				continue;
			for (i = 0; i < qp->q_maxno;i++)
				if(qp->q_proc[i].q_pid)
					(void) kill(qp->q_proc[i].q_pid, SIGHUP);
		}
		else {
			/*
			 * always restart a daemon if told to start
			 */
			if(!(qp->q_state & Q_ENABLE))
				continue;
			qp->q_state &= ~Q_TIMEOUT;
			restartd(qp);
		}
	}
}

/* routine to load table from the file. Only done once so very inefficient.
 * basic job is to fill out the various structures
 * ASSUME:- host name is not split onto two lines with a backslash
 *
 * args not used
 *
 * Two stage process - find master entries, then find subordinates
 * Double scan through the tables, but not that expensive as we don't
 * do it often.
 */

/* ARGSUSED */
gettable(when)
{
	register struct queues  *qp;
	register struct QUEUE   *Qp;
	register char   *q;
	char    xbuff[100];

	/* read in the table */

	for(Qp = QUEUES ; Qp->Qname ; Qp++){
		if (Qp->Qmaster != NULL)
			continue;
		for(Q_SCAN)
			if(qp->q_Qp == Qp)
				break;
		if(!qp){
			qp = (struct queues *)malloc( sizeof(*qp) );
			if(qp == (struct queues *)NULL){
				logger(LOG_ERR, "Out of core\n");
				exit(2);
			}
			/*
			 * Must now make an entry for the queue
			 */
			qp->nextq = qstart;
			qstart = qp;
			qp->q_name = NULL;
			qp->q_rfile = NULL;
			bzero ((char *)qp -> q_proc, sizeof(qp->q_proc));
		}
		if( (q = Qp->Qdir) == NULL)
			q = Qp->Qname;
		if(*q != '/')
			sprintf(xbuff, "%s/%s", NRSdqueue, q);
		else
			(void) strcpy(xbuff, q);
		if(qp->q_name != NULL)
			free(qp->q_name);
		qp->q_name = storestr(xbuff);
		sprintf(xbuff, "%s/%s", qp->q_name, BCONTROL);
		if(qp->q_rfile != NULL)
			free(qp->q_rfile);
		qp->q_rfile = storestr(xbuff);
		qp->q_state = 0;
		qp->q_maxno = 1;
		qp->q_curno = 0;
		qp->q_proc[0].q_pid = 0;
		qp->q_proc[0].q_Qp = Qp;
		qp->q_proc[0].q_name = qp->q_name;
		qp->q_mtime = 0;
		qp->q_Qp = Qp;
		logger (LOG_DEBUG, "+Q %s\n", qp->q_name);
	}
	/* now add parallel queues */
	for(Qp = QUEUES ; Qp->Qname ; Qp++){
		if (Qp->Qmaster == NULL)
			continue;
		qp = getqbyname (Qp->Qmaster);
		if (qp == NULLQP) {
			logger (LOG_WARNING, "Can't locate queue %s\n",
				Qp->Qname);
			continue;
		}
		if (qp -> q_maxno >= MAXSQ) {
			logger (LOG_WARNING,
				"Too many sub queues - %s ignored\n",
				Qp->Qname);
			continue;
		}
		qp -> q_proc[qp->q_maxno].q_pid = 0;
		qp -> q_proc[qp->q_maxno].q_Qp = Qp;
		if( (q = Qp->Qdir) == NULL) {
			logger (LOG_ERR,
				"Must specify a directory for queue %s - ignored\n",
				Qp->Qname);
			continue;
		}

		if(*q != '/')
			sprintf(xbuff, "%s/%s", NRSdqueue, q);
		else
			(void) strcpy(xbuff, q);
		if (qp -> q_proc[qp->q_maxno].q_name)
			free (qp -> q_proc[qp->q_maxno].q_name);
		qp -> q_proc[qp->q_maxno].q_name = storestr(xbuff);
		qp -> q_maxno ++;
		logger (LOG_DEBUG, "+Q %s (%s)\n", xbuff, qp -> q_name);
	}
}

/* deal with the listener dieing */

listenp(pid,status)
{
	register struct listen *lp;

/* find the process in the internal tables */

	for(L_SCAN)
		if(lp->li_pid == pid)
			break;
	if(!lp)                         /* Not found !! Help !! */
		return(0);

	lp->li_pid = 0;                 /* kill it off */

	if( status  == (((-1)<<8) & 0xFF00) )
		lp->li_state |= L_TOSTOP;
	else  if(!closedown)
		logger(LOG_INFO, "Listener %s (%s) returned %d\n",
				(lp->li_Lp->Lprog ? lp->li_Lp->Lprog : QPROC),
				lp->li_Lp->Lname, status);

	if(closedown){                  /* system is dieing */
		lp->li_state = 0;
		return(1);
	}

	if(!(lp->li_state & L_ENABLE) || (lp->li_state & L_TOSTOP)){
		lp->li_state = 0;       /* should die */
		return(1);
	}

	startli(lp);    /* restart the listener */
	return(1);
}

/* start up a listener */

startli(lp)
struct listen *lp;
{
	char    xb[150];
	register pid;
	register struct LISTEN  *Lp = lp->li_Lp;

	if(closedown){
		lp->li_state = 0;
		return;
	}

	while((pid = fork()) < 0);

	if(pid ==0){
		/* kiddy time */
		if(Lp->Lprog == NULL)
			Lp->Lprog = QPROC;
		if(*Lp->Lprog != '/')
			sprintf(xb, "%s/%s", BINDIR, Lp->Lprog);
		else
			(void) strcpy(xb, Lp->Lprog);

		execl(xb, Lp->Lprog, Lp->Lname, 0);
		logger(LOG_ERR, "exec of %s failed\n", xb);
		_exit(-1);
		/*NOTREACHED*/
	}
	/* father */
	lp->li_pid = pid;
}

/* Cycle debug level: user -> high -> off -> user -> .. */

set_debug()
{	int newdebug;
	int olddebug = debug;

	if (debug == user_debug && user_debug != high_debug)
					newdebug = high_debug;
	else if (debug == high_debug)	newdebug = no_debug;
	else				newdebug = user_debug;

	logger(LOG_INFO, "Debug %d -> %d\n", olddebug, newdebug);
	debug = newdebug;
	logger(LOG_INFO, "Debug %d -> %d\n", olddebug, newdebug);
}
/* read in the Q process configuration tables */

read_table(sig)
{
	register struct listen *lp;
	struct  LISTEN  *Lp;

	/* get the Q processes */

	for(Lp = LISTENS ; Lp->Lname ; Lp++){
		for(L_SCAN)
			if(strcmp(lp->li_Lp->Lname, Lp->Lname) == 0)
				break;
		if(!lp){        /* create a new listener slot */
			lp = (struct listen *) malloc( sizeof(* lp) );
			lp->li_nli = lstart;
			lstart = lp;
			lp->li_pid = 0;
			lp->li_state = 0;
			lp->li_Lp = Lp;
		}

		if(islenable(lp)){
			lp->li_state |= L_ENABLE;
			lp->li_state &= ~L_TOSTOP;
			if((sig ==0 || sig==SIGSYS) && !lp->li_pid)
				startli(lp);
		}
		else  {
			lp->li_state &= ~L_ENABLE;
			if(lp->li_pid){
				lp->li_state |= L_TOSTOP;
				if(sig == SIGIOT)
					(void) kill(lp->li_pid, SIGHUP);
			}
		}
		lp->li_state |= L_LOOKED;
	}       /* end of while */

	for(L_SCAN){
		if(!(lp->li_state & L_LOOKED))
			if(sig == SIGIOT && lp->li_pid)
				(void) kill(lp->li_pid, SIGHUP);
		lp->li_state &= ~L_LOOKED;
	}
}

#ifdef JTMP
/* deal with jtmp processes */

startjtmp()
{
	struct  stat    st;
	int     pid;
	char    tbuff[256];

	if(closedown || jtmp_pid)
		return;
	if(stat(JTMPdir, &st) < 0 || (st.st_mode & S_IEXEC) == 0)
		return;
	while( (pid = fork()) == -1);
	if(pid == 0){
		if(!JTMPproc)
			JTMPproc = DJTMP;
		if(*JTMPproc != '/')
			sprintf(tbuff, "%s/%s", BINDIR, JTMPproc);
		else
			(void) strcpy(tbuff, JTMPproc);

		execl(tbuff, JTMPproc, 0);
		logger(LOG_ERR, "exec of %s failed\n", tbuff);
		exit(-1);
	}
	jtmp_pid = pid;
}

jtmpp(pid, status)
{
	jtmp_pid = 0;
	if(status == 0 || closedown || status == (((-1)<<8)&0xFF00) )
		return;
	logger(LOG_INFO. "JTMP process died with %d\n", status);
	startjtmp();
}
#endif JTMP

/* store a string. returning a pointer to it */

char *
storestr(xstr)
char *xstr;
{
	char    *l,*strcpy();
	l = malloc(strlen(xstr) + 1);
	if(l == NULL){
		logger(LOG_ERR, "Out of core\n");
		exit(2);
	}
	return(strcpy(l,xstr));
}

/* routine to log errors. */

/*VARARGS2*/
logger(level, fmt,arg1,arg2,arg3)
int	level;
char    *fmt;
{
	static	long	last_time;
	static	long	last_open;
	static	int	last_level;
	static	FILE    *logfile = (FILE *) 0;
	FILE	*to_be_closed	= (FILE *) 0;
	int	close_logfile	= 0;
	long    thistime;
	char    *ctime();

	if (!debug) if (level >= LOG_DEBUG || !logf) return;

	(void) time(&thistime);

	/* If the log file has been open for a while, try to re-open it */
	if (logfile && logf && (thistime - last_open) > reopen_interval)
	{	to_be_closed = logfile;
		fprintf(logfile, "[%d]+%3d %-19.19s: try to close %x (%d)\n",
			level,
			thistime - ((last_time) ? last_time : thistime),
			ctime(&thistime),
			logfile, fileno(logfile));
		fflush(logfile);
		logfile = (FILE *) 0;
	}

	if (!logfile)
	{	last_open = thistime;
		if (debug && ! logf)	logfile = stderr;
		else if 	(logfile = fopen(logf,"a"))
		{	/*ZAP logfile after use if we can't guarentee append*/
#ifdef	FOPEN_A_APPENDS
			fprintf(logfile, "[%d]+%3d %-19.19s: opened %s\n",
				level,
				thistime - ((last_time) ? last_time : thistime),
				ctime(&thistime), logf);
#else	FOPEN_A_APPENDS
#ifdef	F_GETFL
#ifdef	F_SETFL
#ifdef	FAPPEND
			int l;
			if ((l=fcntl(fileno(logfile), F_GETFL, 0)) != -1 &&
			    fcntl(fileno(logfile), F_SETFL, l | FAPPEND) != -1)
			fprintf(logfile, "[%d]+%3d %-19.19s: opened %s\n",
				level,
				thistime - ((last_time) ? last_time : thistime),
				ctime(&thistime), logf);
			else
#endif	FAPPEND
#endif	F_SETFL
#endif	F_GETFL
				close_logfile = 1;
#endif	FOPEN_A_APPENDS
			if (to_be_closed)	fclose(to_be_closed);
		}
		else	/* open failed -- try to back off to old file */
		{	if (!to_be_closed)
			{	if (debug)	logfile = stderr;
				else		return;
			}
			logfile = to_be_closed;
			to_be_closed = (FILE *) 0;
			fprintf(logfile,
				"[%d]+%3d %-19.19s: reopen %s failed %d\n",
				level,
				thistime - ((last_time) ? last_time : thistime),
				ctime(&thistime), logf, errno);
		}
	}

	if (level == last_level && (thistime - last_time < 10) && debug > 1)
		fprintf(logfile," +%d: ", thistime - last_time);
	else	fprintf(logfile,"[%d]+%3d %-19.19s: ",
			level, thistime - ((last_time) ? last_time : thistime),
			ctime(&thistime));
	fprintf(logfile,fmt,arg1,arg2,arg3);

	if (close_logfile)
	{	fclose(logfile);
		logfile = (FILE *) 0;
	}
	else	fflush(logfile);

	last_time = thistime;
	last_level = level;
}

/*
 * is a queue enabled ?
 */
isqenable(qp)
struct  queues  *qp;
{
	struct  stat    qstat;

	if(stat(qp->q_name, &qstat) < 0)
		return(0);
	if(qstat.st_mode & S_IEXEC)
		return(1);
	return(0);
}

/*
 * is a listener enabled ?
 */
islenable(lp)
struct  listen  *lp;
{
	char    xbuff[100];
	register struct LISTEN  *Lp = lp->li_Lp;

	if(*Lp->Lname != '/')
		sprintf(xbuff, "%s/.L.%s", NRSdqueue, Lp->Lname);
	else
		(void) strcpy(xbuff, Lp->Lname);
	return(access(xbuff, 0) == 0);
}


readbfile(qp)
struct  queues  *qp;
{
	b_fd = open(qp->q_rfile, 0);
	if(b_fd < 0){
		logger(LOG_WARNING, "Bfile open failed %d\n", errno);
		return(-1);
	}
	if(read(b_fd, (char *)&Bfile, sizeof(Bfile)) != sizeof(Bfile)){
		(void) close(b_fd);
		logger(LOG_WARNING, "Bfile read failure\n");
		return(-1);
	}
	(void) close(b_fd);
	if(Bfile.b_nhosts < 0 || Bfile.b_nhosts > MAXBHOSTS){
		logger(LOG_WARNING, "Bfile corrupt %d hosts)\n",
			Bfile.b_nhosts);
		/*
		 * patch it back together again
		 */
		for(Bfile.b_nhosts = MAXBHOSTS-1; Bfile.b_nhosts ;)
			if(Bfile.b_hosts[Bfile.b_nhosts].b_host[0])
				break;
			else
				Bfile.b_nhosts--;
		(void) writebfile(qp, 1); /* write the bfile again */
	}
	Lastbp = &Bfile.b_hosts[Bfile.b_nhosts];
	return(0);
}

writebfile(qp, tocreate)
struct  queues  *qp;
{
	if(tocreate)
		b_fd = creat(qp->q_rfile, (0660) | 0444);
	else
		b_fd = open(qp->q_rfile, 1);
	if(b_fd < 0){
		logger(LOG_WARNING, "Bfile write open failed %d\n", errno);
		return(-1);
	}
	if(write(b_fd, (char *)&Bfile, sizeof(Bfile)) != sizeof(Bfile)){
		(void) close(b_fd);
		logger(LOG_WARNING, "Bfile write failure\n");
		return(-1);
	}
	(void) close(b_fd);
	return(0);
}

cleanbfile(qp)
struct  queues  *qp;
{
	register struct backoff *xp, *bp;
	static  struct  backoff ZERO;   /* a zero Backoff struct */
	long    t;

	if( (b_fd = open(qp->q_rfile, 0)) < 0){
		logger(LOG_WARNING, "Bfile open in clean Bfile failed %d\n",
			errno);
		return(-1);
	}
	if(read(b_fd, (char *)&Bfile, sizeof(Bfile)) != sizeof(Bfile)){
		(void) close(b_fd);
		logger(LOG_WARNING, "Bfile read failure in clean Bfile\n");
		return(-1);
	}
	(void) close(b_fd);
	(void) time(&t);
	for(xp = Bfile.b_hosts,bp= xp ; bp < &Bfile.b_hosts[MAXBHOSTS];bp++)
		if(bp->b_host[0] != 0 && bp->b_hc.h_nextattempt >= t)
			*xp++ = *bp;    /* got an entry we want to save */

	Bfile.b_nhosts = xp - Bfile.b_hosts;
	Lastbp = xp;
	for(; xp < &Bfile.b_hosts[MAXBHOSTS] ; xp++) /* zero all the rest */
		*xp = ZERO;
	if(writebfile(qp, 0) >= 0)
		return(0);
	return(writebfile(qp, 1));
}

Queues *getqbypid(pid)
register int pid;
{
	register Queues *qp;
	register int i;

	for(Q_SCAN) {
		for (i = 0; i < qp -> q_maxno; i++)
			if(qp->q_proc[i].q_pid == pid)
				return qp;
	}
	return NULLQP;
}

Queues	*getqbyname(s)
char	*s;
{
	register Queues *qp;

	for(Q_SCAN) {
		if (strcmp (qp -> q_Qp -> Qname, s) == 0)
			return qp;
	}
	return NULLQP;
}

zapqpid (qp, pid)
Queues	*qp;
int	pid;
{
	register int i;

	for (i = 0; i < qp -> q_maxno; i++)
		if (qp -> q_proc[i].q_pid == pid) {
			qp -> q_proc[i].q_pid = 0;
			qp -> q_curno --;
			return;
		}
	logger (LOG_INFO, "Pid %d not found in queue", pid);
}
