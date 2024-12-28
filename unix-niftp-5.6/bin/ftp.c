#ifndef	lint			/* unix-niftp bin/ftp.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/bin/RCS/ftp.c,v 5.6.1.7 1993/05/10 13:56:48 pb Rel $";
#endif	lint

/*
 * P process daemon
 *
 * $Log: ftp.c,v $
 * Revision 5.6.1.7  1993/05/10  13:56:48  pb
 * Distribution of Apr93SunybytsdPPLDYbAANSICC: Sun YBTSD + PP LD_ + YuckBucked ANSI CC preliminary HACK
 *
 * Revision 5.6.1.5  1992/10/17  05:20:21  pb
 * allow aliases to be passed in to ftp.c -- e.g. PP wanting to tailor channel
 *
 * Revision 5.6.1.4  1992/10/17  05:17:42  pb
 * chack if hostname is same as short form OR long form ...
 *
 * Revision 5.6.1.3  1992/02/07  14:29:59  pb
 * change some of the locking code log msgs L_GENERAL -> L_10
 * add code for ordered queues
 *
 * Revision 5.6.1.2  1991/11/18  13:22:47  pb
 * Use L_MAJOR_COM rather than L_GENERAL to log starting & stopping.
 *
 * Revision 5.6  1991/06/07  16:59:57  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  14:00:59  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.7  90/04/20  11:27:49  pb
 * lot of work on make queue building faster
 * 
 * Revision 5.5  90/03/30  14:33:28  pb
 * Minor mods from jpo for PP debug mode
 * 
 * Revision 5.3  89/07/16  12:02:21  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:33:47  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:20:05  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:32:15  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.5  88/01/29  07:41:14  pb
 * Distribution of Jan88ReleaseMod1: JRP fixes - tcccomm.c ftp.c + news sucking rsft.c + makefiles
 * 
 * Revision 5.0.1.4  88/01/28  06:08:38  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:22:29  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:41:56  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2.1.1  87/09/28  11:16:39  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.5  87/06/30  14:04:21  pb
 * convert to new logging code.
 * don't zap whichhost !
 * 
 * Revision 5.4  87/05/31  18:31:20  pb
 * Resetegid to the ORIGINAL egid, not gid (and uid ...)
 * Add debug info for sun problems ...
 * 
 * Revision 5.3  87/05/31  18:23:34  pb
 * general tidy-up ....
 * 
 * Revision 5.1  87/04/12  09:08:53  pb
 * Add nfs fixes.
 * 
 * Revision 5.0  87/03/23  03:18:02  pb
 * As from wja@nott.cs
 * 
 * Revision 5.0  87/03/23  03:18:02  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "ftp.h"
#include "stat.h"
#include "infusr.h"
#include "retry.h"
#include "nrs.h"
#include "files.h"
#include "jtmp.h"
#include "../version.h"
#include <stdio.h>

#if	(defined(F_APPEND) && !defined(F_GETFL)) || defined(FCNTL)
#include <fcntl.h>
#endif

#ifndef	O_RDWR
# include <sys/file.h>
#endif	O_RDWR

#ifndef	ORDERED
#define	ORDERED	0
#endif
#ifndef	FIRST_WAIT
#define	FIRST_WAIT	(5 * 60)
#endif
#ifndef	MAX_WAIT
#define	MAX_WAIT	(30 * 60)
#endif
#ifndef	SIG_TYPE
#define	SIG_TYPE void
#endif	/* SIG_TYPE */

#define	NI_RTN_OK	0	/* ni_rtn return this for OK */

struct  passwd  *getpwuid();
#ifdef	SETEUID
int	euid, egid;
#define set_sysids()	{ seteuid(euid); setegid(egid); }
#define set_userids()	{ seteuid(uid); setegid(gid); }
#else	SETEUID
#define set_sysids()
#define set_userids()
#endif	SETEUID

struct  passwd  *getpwnam();
extern	char	*rindex();
extern	char	*index();
extern	char    *ctime();
extern	char 	*strcat ();

int lockfd	= -1;

int ordered	= ORDERED;	/* is this channel delivering in order */
int first_wait	= (5 * 60);	/* how long first sleep should be */
int max_wait	= (30 * 60);	/* max sleep */
#ifdef	PP
int P_pp_spooled = -1;
char * pp_errstring;
#endif	PP
char lockname[ENOUGH];
long lockbyte;
char version[] = VERSION;

/*
 * FILE  ftp.c
 * last changed:  6-Feb-86     put onto 2.8
 *
 * by: Phil Cockcroft
 *
 * The main loop of the P process. Initialise and control the whole system
 *
 * run time parameters:
 *	argv[1]    name of directory queue
 *	argv[2]    [-W<window>[:<pkt>]]
 */

/*
 * If at a low level of the program an error is detected which means the
 * abandonment of the current attempt then the routine will call reset()
 * with 'error' set to an appropriate value ( usually REQSTATE ). On this
 * call the stack will be popped and the return will be as though it came
 * to setexit(). The main loop is then entered and the error handling is then
 * performed.
 */

FILE   *stats;      /* statistics file stream */

/*
 * stats file is always in the default directory
 */

static  char    statfile[ENOUGH];
static  char    logfile[ENOUGH];
static  char    statsfile[ENOUGH];
static  char    errfile[ENOUGH];

struct  bfile   Bfile;
int     b_nhosts;               /* current size of Bfile */
struct  backoff *Curbp;         /* current entry pointer */
struct  backoff *Lastbp;        /* last entry pointer */
struct  backoff *newbp();       /* get a new bp entry */
int     b_changed;              /* file has changed */
int     b_fd;                   /* fd of bfile */
char    b_name[BFSIZE];         /* current hosts name */
long    h_time;                 /* various time objects */
struct  host_entry *Curhp;      /* pointer to current host from database */

#define NEVER           (long)0x7FFFFFFF        /* never on sundays */

#ifdef  UCL_ISID        /* flag to say we are talking to isid */
int     p77flag;        /* only needed for a while at UCL */
#endif

long    data_start,data_end;
long	mail_start;
extern  int errno;

extern	char *strncpy ();

int restricted;
#ifdef	PP
int	pp = 0;

#include "ppp.h"

#define	PP_OK 0
#define PP_NOTOK -1
#define PP_DONE 1
#define	LINESIZE	1024
int	pp_read_it();
int	mtafailure = 0;
#endif	/* PP */

main(argc,argv)
char    **argv;
{
	int     i;
	char    _host[ENOUGH];          /* place to put name of host */
	char    _localname[ENOUGH];     /* somewhere for the localname */
	char    _realname[ENOUGH];      /* somewhere for the realname */
#ifdef JTMP
	char	_jtmpfile[ENOUGH];	/* somewhere for the jtmpfile */
#endif JTMP

	localname = _localname;		/* initialise the pointers */
	hostname = _host;
	realname = _realname;
#ifdef JTMP
	jtmpfile = _jtmpfile;
#endif JTMP
	*localname=0;
	*realname=0;
	*hostname=0;
#ifdef JTMP
	*jtmpfile=0;
#endif JTMP
#ifdef	SETEUID
	euid = geteuid();
	egid = getegid();
#endif	SETEUID

	initprocess(argc,argv);		/* initialise the queue, */
	error=0;                        /* if can't open queue */
	setjmp(rcall);                  /* set the return for longjmp() */
	for(;;){
		int new_fd = -1;
		set_sysids();
		if(error){      /* got an error - rewrite queue here */
			stat_state(S_ERRCLOSE);
			if(tab.status == TRYINGSTATE && try_resuming)
				may_resume = 1;
			L_WARN_1(L_GENERAL, 0, "ERROR so rewriting queue %d\n",error);
			if(stats != NULL) fprintf(stats, "e=%d ", error);

			i = error;
			error =0;       /* so that rwq can exit nicely */
			if(!may_resume)         /* don't keep the docket */
				tab.l_docket = 0;
			rwq(i);
			if(f_fd != -1){         /* close the localfile */
				(void) (*closep)(f_fd);     /* if open */
				f_fd = -1;
			}
			if(*localname && !may_resume){  /* delete localfile */
				set_userids();
				L_WARN_2(L_GENERAL, 0,
					"No resumption so delete %s (%s)\n",
					localname, realname);
				(void) unlink(localname); /* if can't resume*/
				*localname=0;
				set_sysids();
			}
		}
		else if(direction)              /* we have a protocol reset */
		{	L_LOG_0(L_GENERAL, 0, "Re exec");
			goto reexec;            /* or something */
		}
#ifdef PP
		mtafailure = 0;
#endif
		/* start a new transfer */
		fclose(stdout);
		if((freopen(logfile,"a",stdout)) ==NULL)
			freopen("/dev/null","a",stdout);

#ifndef	FOPEN_A_APPENDS
#ifdef	F_GETFL
		else if ((i=fcntl(fileno(stdout), F_GETFL, 0)) != -1)
			fcntl(fileno(stdout), F_SETFL, i | FAPPEND);
#endif	F_GETFL
#endif	FOPEN_A_APPENDS

#ifdef  SETLINEBUF
		setlinebuf(stdout);     /* line buffering for BSD */
#else	/* SETLINEBUF */
#ifdef  SETVBUF
		setvbuf(stdout, NULL, _IOLBF, BUFSIZ);     /* SYS V */
#else	/* SETVBUF */
		setbuf(stdout, NULL);
#endif	/* SETVBUF */
#endif	/* SETLINEBUF */

		if(stats != NULL){
			fclose(stats);
			stats = NULL;
		}
		if(access(statsfile,2) >= 0) /* file can be written*/
		{	if((stats = fopen(statsfile,"a")) == NULL)
			{	L_WARN_2(L_GENERAL, 0,
					"Cannot open stats file %s - %d\n",
					statsfile, errno);
			}
#ifdef	FOPEN_A_APPENDS
#ifdef	F_GETFL
			else if ((i=fcntl(fileno(stats), F_GETFL)) != -1)
				fcntl(fileno(stats), F_SETFL, i | FAPPEND);
#endif	F_GETFL
#endif	FOPEN_A_APPENDS

		}

		set_defval();                     /* re-set default values */
		stat_state(S_FINDP);
		getentry();             /* get an entry from the queue */
		stat_state(S_FOUNDP);
		rwq(TRYINGSTATE);       /* write back the fact that we are */
		if(tab.l_fil_n){                     /* trying this entry  */
			(void) strcpy(realname, ((char *)&tab) + tab.l_fil_n);
			if(stats != NULL)
			{	char *name = realname;
#ifdef	MAIL
				if (ISMAILS(tab.t_flags & T_TYPE))
				{	if ((name = rindex(name, '/')) != NULL)
						name++;
					else	name = realname;
				}
#endif	MAIL
				fprintf(stats, "f=%s ", name);
			}
		}
		else *realname=0;                  /* get realname of file */

		*reason = 0;
		getpdocket();            /* get or generate the docket */
/*
 * get here with a good docket for the transfer. May be null or may be
 * a resume attempt
 */
		if(stats != NULL){
#ifdef	MAIL
			if(ISMAILS(tab.t_flags & T_TYPE) && tab.l_from)
				fprintf(stats, "N=%s ",
					(char *) &tab + tab.l_from);
			else
#endif	MAIL
			if(cur_user)
				fprintf(stats, "n=%s g=%d ",
						cur_user->pw_name, gid);
			else
				fprintf(stats, "u=%d g=%d ", uid, gid);
		}

		/*
		 * set up host address information here,
		 * for reporting to user if things go badly
		 */

		(void) strcpy( hostname, ((char *)&tab) + tab.l_hname);
		stat_name(hostname);
		if(stats != NULL)               /* put in stats file */
			fprintf(stats, "h=%s ", hostname);
		L_LOG_1(L_GENERAL, 0, "Host:- %s\n", hostname);	/* get the host */
#ifdef  UCL_ISID
		/* isid specific */
		if(strncmp(hostname, "isid", 4) == 0)
			p77flag = 1;
		else
			p77flag = 0;
		L_LOG_1(4, 0, "p77flag %s\n", p77flag?"true":"false");
		/* isid end */
#endif
		set_userids();
#ifdef	MAIL
		mailer = 0;
		if(ISMAILS(tab.t_flags & T_TYPE))
		{
#ifdef	PPX
			if (PPTRANS)
			{	mailer++;
				if (!P_pp_spooled) mailer++;
			}
#endif	PPX
			mailer++;
		}
#endif	MAIL

		if(lastmark == INFINITY)	/* resuming after STOP */
			new_fd = f_fd;			/* do nothing */
		else if(!(tab.t_access & ACC_GET)){	/* Transmitting */
			L_LOG_1(L_GENERAL, 0, "Transmitting %s\n",realname);
			(void) strcpy(tofrom,"to");
			if(
#ifdef	PP
				(!(ISPP(tab.t_flags & T_TYPE))) &&
#endif	/* PP */
#ifdef	MAIL
#ifndef	MAILOWNSMAIL
				(!ISMAILS(tab.t_flags & T_TYPE)) &&
#endif	MAILOWNSMAIL
#endif	MAIL
				perms(realname,0,uid,gid) <0
			){
				L_WARN_4(L_GENERAL, 0,
					"%d/%d cannot read local file %s (%x)\n",
					uid, gid, realname, tab.t_flags & T_TYPE);
				error = REJECTSTATE;    /* no permissions */
				continue;
			}
			else {	/* reset errors & reasons */
				ecode = 0;
				*reason = 0;
			}
#ifdef	PP
			if (!pp || P_pp_spooled)
#endif	PP
			if ((new_fd = open(realname,0))< 0){
				L_WARN_2(L_GENERAL, 0, "open error %d on %s\n",
					errno, realname);
				error = REQSTATE;       /* why can't I */
				continue;               /* perms says I can */
			}
		}
		else {                                  /* receiving */
			if(!try_resuming)       /* already got one */
				genlocalname();
			L_LOG_2(L_GENERAL, 0, "Receiving into %s for %s\n",
							localname,realname);
			(void) strcpy(tofrom, "from");
			if(!*localname){
				L_ERROR_0(L_ALWAYS, 0, "Filename has gone\n");
				may_resume = 0;
				try_resuming = 0;
				error = REQSTATE;
			       (void)strcpy(reason,"Internal filename botch");
				continue;
			}
			if(perms(realname,1,uid,gid) < 0){
				L_WARN_3(L_GENERAL, 0,
				      "Insufficient access for %d/%d to %s\n",
					uid, gid, realname);
				error = REJECTSTATE;    /* no permissions */
				may_resume = 0;         /* to access file */
				continue;
			}
			if(try_resuming){ /* should already have localfile */
				if( ( new_fd = open(localname,1)) <0){
					if((new_fd=creat(localname,0600)) <0){
						L_WARN_2(L_GENERAL, 0,
					"No creat localfile %s - %d\n",
							localname, errno);
						error = REQSTATE;
						may_resume = 0;  /* Help !! */
						continue;
					}
					else {
						lastmark = 0;   /* start at */
						rec_mark = 0;   /* begining */
					}                       /* again */
				}
			}
			else  {         /* create the localfile */
				if((new_fd = creat(localname,0600)) < 0){
					L_WARN_2(L_GENERAL, 0, "NO receiving %s - %d\n",
						realname, errno);
					error = REJECTSTATE;       /* can't */
					continue;    /* probarbly read only */
				}                      /* filestore */
			}
		}
		f_fd = new_fd;
		f_access = tab.t_access;        /* set type of access */
		stat_state(S_OPENING);
		if(net_open)         /* put a clock on it */
			starttimer(2*60);
		else if(con_open(hostname)){     /* try to open */
			int     t;
			/*
			 * try again later. Cannot
			 * try less than once / hour
			 */
#ifdef PP
			mtafailure = 1;
#endif
			if(!*reason)
			{	(void) strcat(reason, "connection to ");
				(void) strcat(reason, hostname);
				(void) strcat(reason, " failed");
			}
			stat_state(S_OPENFAILED);
			if(Curbp == NULL)
				Curbp = newbp();
			t = first_wait * ++Curbp->b_hc.h_nback;
			if(t > max_wait || t <= 0) t = max_wait;
			Curbp->b_hc.h_nextattempt = h_time + t;
			b_changed = 1;
			tab.l_nextattmpt = h_time + t;
			error = REQSTATE; /* non fatal */
			if(!try_resuming)
				may_resume =0;
			continue;
		}
/*
 * connection is now open time to talk to the other process
 *  The h_control is done after the sft because o local net problems
 *   When accessing through the gate the error occurs when the write
 *   has been completed and it is returned on the read of the RPOS
 */
		set_sysids();
		stat_state(S_SFT);
		send_sft();               /* send SFT and process reply*/
		stat_val(0);
		stat_state(S_DATA);
		set_userids();
		if(Curbp != NULL){
			Curbp->b_hc.h_nextattempt= 0;
			Curbp->b_hc.h_nback = 0;
			b_changed = 1;
		}
		try_resuming = 0;               /* stop possible illegal */
		if(!may_resume)                 /* resume attempts */
			tab.l_docket = 0;
		rwq(GOSTATE);                   /* say we can do it */
		if(st_of_tran == VIABLE && lastmark != INFINITY){
			send_go();                      /* send the GO */
			direction = (tab.t_access & ACC_GET) ?	RECEIVE :
								TRANSMIT;
			tstate = GOs;           /* set the right state */
			if(ftp_print & L_GENERAL)
				(void) time(&data_start);
	reexec:
			if(direction==TRANSMIT)
				transmit();     /* do the transmission */
			else	receive();      /* receive the file */
			if(ftp_print & L_GENERAL) (void) time(&data_end);
		}
		else if(lastmark == INFINITY && st_of_tran == VIABLE){
			tstate = OKs;      /* failed in termination phase*/
			st_of_tran = TERMINATED;  /* resume at STOP */
		}
		stat_state(S_DONE);
		set_sysids();
		writedocket();           /* log curent state in docket */
		direction =0;           /* stop spurious results */
		close(f_fd);            /* close the localfile */
		f_fd = -1;
		if(net_open)            /* send the stop */
		{
#ifdef	NEWS
			if (ISNEWS(tab.t_flags & T_TYPE) &&
			    (st_of_tran == (TERMINATED | TERMINATED_OK)) &&
			    (f_access & ACC_GET))
			{	
				stat_state(S_PROCESS);
				do_news(realname);
			}
#endif	NEWS
			stat_state(S_AWAITSTOP);
			do_stop();
		}
		if(rej_resume){
			L_WARN_0(L_GENERAL, 0, "Resumption rejected, try again\n");
			error = REQSTATE;
			if(net_open)    /* stop bug in GEC software */
				con_close();
			continue;
		}
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
/*		*whichhost = '\0';	commented out by pb@cam.cl jun 87   */
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
/* ------------------------------------------- WHY ?? --------------------- */
		switch(st_of_tran & ~0xF){	/* report the current state */
		case REJECTED:                  /* rejected state */
			L_WARN_0(L_GENERAL, 0, "File transfer rejected\n");
			if((st_of_tran & 0xf) == REJECTED_RESUME)
				error = REQSTATE;  /* couldn't find docket */
			else if ((st_of_tran & 0xf) == REJECTED_POSS)
			{	int now = time((long *) 0); /* can retry */
				error = REQSTATE;  /* Try again later */
				tab.l_nextattmpt = now + first_wait;
			}
			else	error = REJECTSTATE;    /* total failure */
			continue;
		case ABORTED:                           /* aborted */
			L_WARN_0(L_GENERAL, 0, "transfer aborted\n");
			error = ABORTSTATE;
			if((st_of_tran & 0xff) == ABORTED_POSS)
			{	int now = time((long *) 0); /* can resume */
				error = REQSTATE;  /* Try again later */
				tab.l_nextattmpt = now + first_wait;
				if(net_open) con_close();
			}
			else	may_resume = 0;		/* can't resume */
			continue;
		case TERMINATED:            /* terminated */
			if(st_of_tran == TERMINATED)
				break;
			if(net_open &&
				st_of_tran == (TERMINATED|TERMINATED_MSG) ){
				/* problem at stopack ?? */
				L_WARN_0(L_GENERAL, 0, "Transfer failed on STOPACK\n");
				(void) strcpy(whichhost, "remote");
				error = FABORTSTATE;
				continue;
			}
	/*
	 * here is a big problem. What to do if the state of transfer is no
	 * good. I can't think of why.........
	 */
		default:                        /* viable - problems */
			L_WARN_1(L_GENERAL, 0, "Some sort of error %0x4\n", st_of_tran);
			error = ABORTSTATE;     /* maybe too harsh */
			continue;
		}
		if(f_access==ACC_GJI)                    /* job output */
			start_job(realname);
		tstate = 0;
		stat_state(S_DONE);
		rwq(DONESTATE);         /* inform user of success */
	}                                               /* if I should */
}

#ifdef JTMP
jtmp_fin()
{
	struct  jqent   jq;
	int     fd;

	if(!*jtmpfile)
		return;
	if( (fd = open(jtmpfile, 2)) >= 0){
		if(read(fd, (char *)&jq, sizeof(jq)) == sizeof(jq)){
			if(jq.jtsktype == TASK_DD)
				goto delete;
			(void) lseek(fd, 0L, 0);
			jq.jstate = JB_FTPD;
			(void) write(fd, (char *)&jq, sizeof(jq));
			(void) close(fd);
			*jtmpfile = 0;
			return;
		}
delete:;
		(void) close(fd);
	}
	(void) unlink(jtmpfile);
	*jtmpfile = 0;
}
#endif JTMP

/*
 * routine to initialise the process. Opens log and queue files
 * also the lockfile. Catch signals as needed
 */

DIR     *qfdir;         /* pointer to queue directory */
struct  QUEUE   *Myqueue;

char	optargs[] = "Pp:q:w:";
extern	int optind;
extern	char *optarg;

initprocess(argc,argv)
char    **argv;
{
	register i;
	long    lseek();
	extern SIG_TYPE sig1(),sigabort();
	char    queue[ENOUGH], *q;
	struct  passwd *pw;
	char	*queuename = (char *) 0;

	static  SIG_TYPE (*funcs[NSIG])() = {
#ifdef FULLPP
		sig1, SIG_DFL, SIG_DFL,
#else
		sig1, SIG_IGN, SIG_IGN,
#endif
		sigabort, sigabort, sigabort,
		sigabort, sigabort, SIG_DFL,
		sigabort, sigabort, sigabort,
		SIG_IGN, SIG_IGN, sigabort,
		};

	int c;


	if (!isatty(1)) {
		freopen("/tmp/psun.out","a",stdout);
		freopen("/tmp/psun.err","a",stderr);
	}
	/* printf("psun (%s) started as %d\n", argv[0], getpid()); */
	/* fprintf(stderr, "psun (%s) started as %d\n", argv[0], getpid()); */
	/* { int i; for (i=0; i<argc; i++) printf("%s%s", i?", ":"", argv[i]); printf("\n"); } */
#ifdef	FULLPP
	{	char *name;
		if ((name = rindex(argv[0], '/')) != NULL) name++;
		else	name = argv[0];
		if (!strcmp(name, "greyout\0<---- patch greyout>") ||
		    !strcmp(name, "grey\0<---------- patch grey>"))
			pp++;
	}
#endif	/* PP */

	while ((c = getopt(argc, argv, optargs)) != EOF) switch(c)
	{
	default:
		fprintf(stderr, "%s: bad args for: %s [-%s] [queue]\n",
			*argv, *argv, optargs);
		exit(1);
#ifdef	FULLPP
	case 'P': pp++;						break;
#endif	/* FULLPP */
	case 'q': queuename = optarg;				break;
	case 'p': if ((i = atoi(optarg)) != 0) def_pktsize = i;	break;
	case 'w': if ((i = atoi(optarg)) != 0) def_wndsize = i;	break;
	}

#ifdef	FULLPP
	if (pp)
	{	/* Assume we are called as "greyout -P <pp_init args>".
		 * Note that that INCLUDES argv[0]
		 */
		if (ppp_init(argc, argv) < 0)
		{	fprintf(stderr, "pp init failed\n");
			exit(1);
		}
		/* set the queue name, e.g. for tailoring */
		if (!queuename) queuename = "PP";
		/* Set the read routine to the PP stub */
		readp = pp_read_it;
		/* Reduce the facilities as appropriate */
		{	int i;
			for (i=0; i>=0; i++) switch(sfts[i].attribute & 0xff)
			{
			case FACIL:	sfts[i].ovalue &= ~0x4e; break;
			case 0xFF:	i = -2; break;
			}
		}
	}
	else
#endif	/* FULLPP */
	if (queuename && optind == argc)
		/*empty*/;
	else if (optind != argc -1 || queuename)
	{	fprintf(stderr, "%s: bad args for: %s [-%s] [queue]\n",
			*argv, *argv, optargs);
		exit(1);
	}
	else	queuename = argv[optind];

	if(nrs_init() < 0){
		printf("cannot initialise\n");
		exit(2);
	}

	for(Myqueue = QUEUES ; Myqueue->Qname ; Myqueue++)
		if(strcmp(Myqueue->Qname, queuename) == 0)
			break;

	if(Myqueue->Qname == NULL){
		printf("Unknown queue %s\n", queuename);
		exit(2);
	}
	for(i = 0 ; i < NSIG ; i++)     /* process signals */
		signal(i+1,funcs[i]);
#ifndef FULLPP
#ifdef  SIGTSTP
	(void) signal(SIGTSTP,SIG_IGN); /* stop the STOP signal on vax's */
#endif
#endif
#ifdef  SIGXFSZ
	(void) signal(SIGXFSZ, SIG_IGN);
#endif
	/* should realy be in nrsinit */
	if((pw = getpwnam(FTPuser)) != NULL){
		FTPuid = pw->pw_uid;
	}

#ifndef FULLPP
	for(i = 0 ; i < NFILES ; i++)	/* close all files */
		(void) close(i);        /* it's a daemon isn't it */
#endif
	(void) umask(0);

	ftp_print = Myqueue->Qlevel;

	if (Myqueue->Qbackoff) first_wait = Myqueue->Qbackoff;
	if (Myqueue->Qbackmax) max_wait = Myqueue->Qbackmax;
	if (Myqueue->Qordered) ordered = (Myqueue->Qordered > 0);

	if(! Myqueue->Qlogfile)
		sprintf(logfile, "%s/%s%s", LOGDIR, PLOG, Myqueue->Qname);
	else if (*Myqueue->Qlogfile != '/')
		sprintf(logfile, "%s/%s", LOGDIR, Myqueue->Qlogfile);
	else	(void) strcpy(logfile, Myqueue->Qlogfile);

	/* get stat file -- default is stat.p<name> */
	if(! Myqueue->Qstatfile)
		sprintf(statfile, "%s/%s%s", LOGDIR, PSTT, Myqueue->Qname);
	else if (*Myqueue->Qstatfile != '/')
		sprintf(statfile, "%s/%s", LOGDIR, Myqueue->Qstatfile);
	else	(void) strcpy(statfile, Myqueue->Qstatfile);

	if(Myqueue->Qerrfile) sprintf(errfile,
		(*Myqueue->Qerrfile == '/') ? "%0.0s%s" : "%s/%s",
		LOGDIR, Myqueue->Qerrfile);

	sprintf(statsfile, "%s/%s%s", LOGDIR, PSTAT, Myqueue->Qname);

#ifdef PP
	if (!pp)
#endif
		freopen("/dev/null","r",stdin); /* get a stdin.fd 0 */

	if( (freopen(logfile,"a",stdout))==NULL){
		freopen("/dev/null","w",stdout);
	}
	(void) chmod(logfile,(0660) | 0444);  /* for various reasons */
#ifdef PP
	if (!pp)
#endif /* PP */
	{
		fclose(stderr);
		freopen("/dev/null", "w", stderr);
	}

#ifdef  SETLINEBUF
	setlinebuf(stdout);     /* line buffering for BSD */
#else	/* SETLINEBUF */
#ifdef  SETVBUF
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);     /* SYS V */
#else	/* SETVBUF */
	setbuf(stdout, NULL);
#endif	/* SETVBUF */
#endif	/* SETLINEBUF */

	stat_init(statfile);
	L_LOG_2(L_MAJOR_COM, L_DATE | L_TIME, "%s %s: Daemon started\n",
			argv[0], version); /* entry in log */
	L_LOG_4(L_MAJOR_COM, 0, "monitoring level set to %d (%04x)%s%s\n",
		ftp_print, ftp_print,
		(Myqueue->Qbackoff || Myqueue->Qbackmax) ? " (backoff set)" : "",
		(Myqueue->Qordered) ? " (ordered)" : "");

#ifdef	PP
	if (!pp)
#endif	/* PP */
	{
		if( (q = Myqueue->Qdir) == NULL) q = Myqueue->Qname;

		if(*q != '/')	sprintf(queue, "%s/%s", NRSdqueue, q);
		else		(void) strcpy(queue, q);

		if(chdir(queue) < 0 || (qfdir = opendir(".")) == NULL){
			printf("Cannot open queue\n");
			exit(-1);
		}
		if((b_fd = open(BCONTROL,2)) < 0){
			printf("Cannot open BCONTROL %s\n",BCONTROL);
			exit(-1);
		}
		if(read(b_fd, (char *)&Bfile, sizeof(Bfile)) != sizeof(Bfile))
		{	printf("Read failed on BCONTROL\n");
			exit(-1);
		}
		if(Bfile.b_nhosts < 0 || Bfile.b_nhosts > MAXBHOSTS)
		{	printf("Corrupt BCONTROL file\n");
			exit(-1);
		}
		Lastbp = &Bfile.b_hosts[Bfile.b_nhosts];
		if(access(statsfile,2) >= 0)    /* exists for writing */
			if ((stats=fopen(statsfile,"a"))== NULL) L_WARN_1(
				L_GENERAL, 0,
				"Cannot open stats file %s\n", statsfile);
	}
#ifdef PP
	else {
		Lastbp = &Bfile.b_hosts[0];	/* jpo fix */
		b_fd = qfd = -1;
	}
#endif
}

/*
 * routine to read an entry from the queue directory.
 * Search algorithm.
 *      First pass.
 *          If host is not open just use the first one we come to
 *          which is not to the same host.
 *          If open then search for next entry to go to that host.
 *           (If non found then close the connection ).
 *      Second pass.
 *          Search for any other entry not on this host.
 *      Third pass.
 *          Search for entries to the current host.
 *
 */

#define STRUCTTABSIZE   sizeof(struct tab)


/*
 * Scan the current directory and make a list of daemon files sorted by
 * creation time.
 * Return the number of entries and a pointer to the list.
 */

#define FIRSTHOST       1
#define ANYHOST         2
#define SAMEHOST        3
#define OTHERHOST       4

struct  quent {
	long    q_ttime;
	char    *q_hname;               /* alias of host */
	u_long	q_inode;
	char    q_name[15];             /* ignore 4.2 !! */
	};

struct  hent {
	char    h_name[BFSIZE];        /* alias of host */
	int	h_valid;
	struct	hent	*h_next;
	};

struct  fent {
	struct fent *f_next;	/* Next one in the chain */
	char    *f_hname;	/* remote host */
	long	f_inode;	/* Inode of this file */
	long	f_hashval;	/* File name hash value */
	long    f_next_try;	/* time to next try this file */
	int	f_state;	/* state at last inspection */
	int	f_tflags;	/* flags from tab info */
	int	f_valid;	/* Is this entry valid */
	};

#define	H_valid		0x01
#define	H_unknown	0x02
#define	H_disabled	0x04
#define	H_waiting	0x08
#define	H_locked	0x10
#define	H_unlocked	0x20
#define	H_never		0x40
#define	H_dont_call	(H_disabled | H_never /* | H_locked */)
#define	H_off_mask	(~ (H_valid | H_unlocked))
#define	H_anylock	(H_locked | H_unlocked)

#define	F_valid		1

#define MAXFENT 200	/* Open hash chain size for the file entries*/
#define MAXHENT 50	/* Open hash chain size for the host entries*/
#define MAXQENT 50	/* maximum number of entries in the queue */

#define MAXQTRANS 50    /* maximum number of transfers to the same host */
			/* at any one time. Used mainly for transfers to UCL*/

struct  fent    *fentrys[MAXFENT];      /* entries in the queue */
struct  hent    *hentrys[MAXHENT];      /* entries in the queue */
struct  quent   *qentrys[MAXQENT];      /* entries in the queue */
struct  quent   **qend = qentrys;       /* pointer to end of queue */
struct  quent   **curqent;              /* pointer to current queue entry */
int     ninq;                           /* number of elements in queue */

static  hcount;                 /*a count of the number of transfers*/
static  tabentry;               /*set if got an entry */
static  mailtrans;              /* set if address being sent to is for mail */
static  maxftptrans;            /* max numb trans / TS connection */
static  linear;

static int distinct_hosts = 0;
static int distinct_files = 0;
static int read_hosts = 0;
static int read_files = 0;

/* ------------------------- host hashing code -------------------------- */
static int h_hash(host)
char *host;
{
	int rc = 13;
	while (*host) rc = (rc * 26) + (*host++ - 'a');
	rc %= MAXHENT;
	if (rc < 0) rc += MAXHENT;
	return rc;
}


static struct hent * find_host(host, prhp, create, getinfo)
char *host;
struct  host_entry **prhp;
int create;
int getinfo;	/* <0 -> force reread, 0 -> dont read, >0 -> read if ~valid */
{
	int hash = h_hash(host);
	struct hent **hptr = &(hentrys[hash]);
	struct  host_entry      *rhp;
	register struct backoff *bp;
	long stt = time((long *) 0);

	if (prhp) *prhp = (struct host_entry *) 0;
	L_LOG_3(L_10, 0, "find_host: %-15s gave %d (%x)\n", host, hash, *hptr);
	while (*hptr && strcmp((*hptr)->h_name, host))
		hptr = &((*hptr)->h_next);

	if (! *hptr && create)
	{	*hptr = (struct hent *) malloc(sizeof(struct hent));
		distinct_hosts ++;

		if (! *hptr) return *hptr;
		bzero(*hptr, sizeof(struct hent));
		strncpy((*hptr)->h_name, host, BFSIZE-1);
		(*hptr)->h_name[BFSIZE-1] = '\0';
	}

	if (getinfo == 0 || (getinfo > 0 && ((*hptr)->h_valid & H_valid)))
		return *hptr;

	(*hptr)->h_valid |= H_valid;
	if( (rhp = dbase_get(host)) == NULL){
		L_WARN_1(L_GENERAL, 0, "Cannot find host %s in dbase\n",
				host);
		(*hptr)->h_valid |= H_unknown;
	}
	if (prhp) *prhp = rhp;
	if(rhp && rhp->n_disabled){
		L_WARN_1(L_GENERAL, 0, "Host %s disabled\n", host);
		(*hptr)->h_valid |= H_disabled;
	}
	for(bp = Bfile.b_hosts ; bp < Lastbp ; bp++)
		if(strncmp(host, bp->b_host, BFSIZE-1) == 0)
			break;
	if(bp < Lastbp && bp->b_hc.h_nextattempt > stt){
		L_LOG_3(L_NOTRETRY, 0,
			"Not retrying %s yet (%d:%02d)\n",
			host,
			(bp->b_hc.h_nextattempt - stt) /60,
			(bp->b_hc.h_nextattempt - stt) %60);
		(*hptr)->h_valid |= H_waiting;
	}
	if (strcmp(host, lockname) && test_lock(host) < 0)
		(*hptr)->h_valid |=  H_locked;
	else	(*hptr)->h_valid &= ~H_locked;
	if (rhp && !((*hptr)->h_valid & H_disabled))
	{	int i;
		for(i = 0 ; i < rhp->n_nets ; i++)
			if(rhp->n_addrs[i].net_name)
				break;
		if(i >= rhp->n_nets)
			i = 0;
		if (TRANS_CALL_VAL(rhp->n_addrs[i].n_ftptrans) ==
			TRANS_CALL_NEVER)
			(*hptr)->h_valid |=  H_never;
		L_LOG_4(L_NOTRETRY, 0, "%d gave %x:%x so %x\n",
			i, rhp->n_addrs[i].n_ftptrans,
			TRANS_CALL_VAL(rhp->n_addrs[i].n_ftptrans),
			(*hptr)->h_valid);
	}
	read_hosts++;
	L_LOG_4(L_10, 0, "%-15s gave %6x %2x (%s)\n",
		host, *hptr, (*hptr)->h_valid, (*hptr)->h_name);
	return *hptr;
}
/* ------------------------- file hashing code -------------------------- */
static int f_hash(file, inode, pval)
char *file;
u_long inode;
long *pval;
{
	int rc = inode;
	while (*file) rc = (rc * 26) + (*file++ - 'a');
	if (pval) *pval = rc;
	rc %= MAXFENT;
	if (rc < 0) rc += MAXFENT;
	return rc;
}


static struct fent * find_file(file, inode, pfd, create, getinfo)
char *file;
u_long inode;
int *pfd;
int create;
int getinfo;	/* <0 -> force reread, 0 -> dont read, >0 -> read if ~valid */
{
	long hashval;
	int hash = f_hash(file, inode, &hashval);
	struct fent **fptr = &(fentrys[hash]);
	struct hent *hptr;
	int fd;
	int nb;

	L_LOG_4(L_10, 0, "find_file(%-9s, %6x, %x, %d",
		file, inode, pfd, create);
	L_LOG_3(L_10, L_CONTINUE, ", %2d)=%4d (%x)\n",
 		getinfo, hash, *fptr);
	if (pfd)	*pfd = -1;
	while (*fptr && 
		((*fptr)->f_inode != inode || (*fptr)->f_hashval != hashval))
		fptr = &((*fptr)->f_next);

	if (!*fptr && create)
	{	*fptr = (struct fent *) malloc(sizeof(struct fent));
		distinct_files ++;
		if (!*fptr)	return *fptr;

		bzero(*fptr, sizeof(struct fent));
		(*fptr)->f_inode = inode;
		(*fptr)->f_hashval = hashval;
	}
	if (getinfo == 0 || (getinfo > 0 && ((*fptr)->f_valid & F_valid)))
		return *fptr;
	if( (fd = open(file, 2)) < 0)
	{
		L_LOG_3(L_ALWAYS, 0, "Open(%s) gave %d (%d)\n", file,
			fd, errno);
		return *fptr;
	}
	if (pfd) *pfd = fd;
	if((nb=read(fd, (char *)&tab, STRUCTTABSIZE)) != STRUCTTABSIZE){
		L_DEBUG_1(1, 0, "Processing %s ", file);
		L_DEBUG_4(1, L_CONTINUE, "read on %d gave %d (%d/%d)\n", fd, nb, STRUCTTABSIZE, errno);
		L_DEBUG_4(1, 0, "got %d/%d, %d/%d\n", getuid(), geteuid(), getgid(), getegid());
		(void) close(fd);
		return *fptr;
	}
	if (! pfd) (void) close(fd);
	(*fptr)->f_state = tab.status;
	(*fptr)->f_next_try = tab.l_nextattmpt;
	(*fptr)->f_tflags = tab.t_flags;
	if (!(hptr = find_host((char *)&tab + tab.l_hname, 0, 1, 1)))
	{	L_LOG_1(L_ALWAYS, 0, "Failed to find %s\n",
			(char *)&tab + tab.l_hname);
		return *fptr;
	}
	(*fptr)->f_hname = hptr->h_name;
	(*fptr)->f_valid |= F_valid;
	read_files++;
L_LOG_2(L_10, 0, "find_file(%-9s) gave %x\n", file, *fptr);
	return *fptr;
}


static void list_entries(qentrys, ninq)
struct quent **qentrys;
int ninq;
{
	int i;
	L_LOG_1(L_10, 0, "list_entries given %d entries:\n", ninq);
	for (i=0; i<ninq; i++)
	L_LOG_4(L_10, 0, "%-9s %-15s %4x %x\n",
		qentrys[i]->q_name,
		qentrys[i]->q_hname,
		qentrys[i]->q_inode,
		qentrys[i]->q_ttime);
}
static void sort_by_host(qentrys, ninq)
struct quent **qentrys;
int ninq;
{
	L_LOG_1(L_10, 0, "sort_by_host given %d entries\n", ninq);
	list_entries(qentrys, ninq);
	return;
}


buildq(dirp, whichsearch, hname)
int     whichsearch;            /* type of search required */
char    *hname;                 /* various other search algorithms */
DIR     *dirp;
{
	register struct direct *d;
	register struct quent  **qp;
	struct  stat    statbuf;
	long    stt;
	int     ishost;
	int     possibles;
	static int compar();
	char    *hnm;
	long    dirpos;
	static	entries_this_time, entries_last_time = -1;
	int	save_entries_this_time;
	struct hent *hptr;
	long	start_t = time((long *) 0);
	int	last_files = read_files;
	int	last_hosts = read_hosts;

	checkbf(0);
	if(entries_last_time > 1000)
		linear = 1;
	else
		linear = 0;
	dirpos = telldir(dirp);
	save_entries_this_time = entries_this_time;
again:
	seekdir(dirp, dirpos);
	possibles = 0;
	(void) time(&stt);
	curqent = NULL;
	for(qp = qentrys ; qp < qend ; *qp++ = NULL)    /* free  some space */
		if(*qp)
			free(*qp);
	ninq = 0; qend = qentrys;

	while ((d = readdir(dirp)) != NULL) {
		struct fent *fptr;
		if (d->d_name[0] != 'q')
			continue;       /* daemon queue files only */
		/*
		 * if going to same host don't bother to get more than
		 * we can do at any one time
		 */
		if(possibles > MAXQENT && ninq > 0 || 
			(possibles > 2*MAXQENT && whichsearch != ANYHOST))
			break;
		if(whichsearch == SAMEHOST && ninq + hcount > maxftptrans)
			break;
		if (!(fptr = find_file(d->d_name, d->d_fileno, (int *)0, 1, 1)))
						continue;	/* LOG? */
		if (!(fptr->f_valid & F_valid))	continue;	/* LOG? */
		entries_this_time++;
		switch(fptr->f_state){
		case XDONESTATE:
			L_ERROR_0(L_ALWAYS, 0, "Old done state\n");
			printf("Old done state\n");
		case DONESTATE:
		case REJECTSTATE:
		case ABORTSTATE:
		case CANCELSTATE:
			if(statbuf.st_mtime + 60*60 < stt || linear){
#ifdef JTMP
				if(ISJTMP(fptr->f_tflags & T_TYPE)
						      && tab.l_jtmpname){
					/* we have a jtmp transfer.
					 * delete any old files associated
					 * with it.
					 */
					hnm = (char *)&tab + tab.l_jtmpname;
					if(tab.status != DONESTATE)
						(void) unlink(hnm);
				}
#endif JTMP
#ifdef  DIR_COMPACTS
			/* What if directory changes size.
			 * We could have problems !! Hence the goto again
			 * below. But only if the unlink succeeds.
			 * Otherwise we get into a race condition.
			 */
				L_WARN_2(L_GENERAL, 0,
					"State %x so delete %s\n",
					fptr->f_state, d->d_name);
				if(unlink(d->d_name) == 0){
					entries_this_time = save_entries_this_time;
					goto again;
				}
#else	DIR_COMPACTS
				L_WARN_2(L_GENERAL, 0,
					"State %x so delete %s\n",
					fptr->f_state, d->d_name);
				(void) unlink(d->d_name);
#endif	DIR_COMPACTS
			}
			continue;
		default:
			break;
		}

		/* Check that we should be trying this one .... */
		if (fptr->f_next_try > stt)
		{
			L_LOG_3(L_NOTRETRY, 0,
				"Not retrying %s yet (%d:%02d)\n",
				d->d_name,
				(fptr->f_next_try - stt) /60,
				(fptr->f_next_try - stt) %60);
			continue;
		}

		possibles++;    /* say we have an entry */

		/*
		 * Now check on which searching algorithm we should use
		 */
		hnm = fptr->f_hname;
		ishost = strcmp(hnm, hname);
		switch(whichsearch){
		case FIRSTHOST:
		case ANYHOST:				break;
		case SAMEHOST:	if( ishost) continue;	break;
		case OTHERHOST:	if(!ishost) continue;	break;
		}
		/*
		 * Now check to see if this host is enabled
		 */
		Curhp = NULL;
		hptr = find_host(hnm, 0, 1, whichsearch != SAMEHOST && !linear);
		if (!hptr)
		{
			L_LOG_1(L_ALWAYS, 0, "Failed to find %s\n", hnm);
			continue;
		}
		if (hptr->h_valid & H_dont_call) continue;	/* LOG? */
		if(whichsearch != SAMEHOST && !linear &&
			(hptr->h_valid & H_off_mask))
		{
			L_LOG_3(L_NOTRETRY, 0, "Omit %s for %s as %x\n",
				d->d_name, hnm, hptr->h_valid);
			continue;
		}
		*qend = (struct quent *)malloc(sizeof(struct quent));
		if(*qend == NULL){
			L_WARN_0(L_GENERAL, 0, "Out of core\n");
			break;
			/* get out now while the going is good */
		}
		(*qend)->q_hname = hptr->h_name;
		(*qend)->q_inode = fptr->f_inode;

		/* Do NEWS first come, first servered, others round robin */
		(*qend)->q_ttime = (ISNEWS(fptr->f_tflags & T_TYPE) &&
			tab.t_queued) ? tab.t_queued : statbuf.st_mtime;

		(void) strcpy((*qend)->q_name,d->d_name);
		qend++;
		ninq++;
		if(ninq < MAXQENT-1)   /* still lots of space */
			continue;
		/*
		 * sort the entry's so that we loose the last one.
		 */
		if(!linear)
			qsort(qentrys, ninq, sizeof(struct quent *), compar);
		free( (char *) *--qend);
		*qend = NULL;
		ninq--;
		break;  /* for now */
	}
	if(d == NULL){
		entries_last_time = entries_this_time;
		entries_this_time = 0;
#ifdef	_42
		closedir(dirp);
		if((qfdir = opendir(".")) == NULL){
			printf("Cannot reopen directory\n");
			stat_close((char *) 0);
			unlock_byte(lockname, lockfd, lockbyte);
			exit(78);
		}
#else
		rewinddir(dirp);
#endif
	}
	if(ninq && !linear)
		qsort(qentrys, ninq, sizeof(struct quent *), compar);
	sort_by_host(qentrys, ninq);
	if(!ninq && possibles)
		ninq--;
	L_LOG_4(L_10, L_TIME, "buildq(%d, %s) gave %d (h=%d",
		whichsearch, (hname) ? hname : "<hname not set>",
		ninq, distinct_hosts);
	L_LOG_4(L_10, L_CONTINUE, "/%d f=%d/%d) after %d sec\n",
		read_hosts - last_hosts,
		distinct_files, read_files - last_files,
		time((long *) 0) - start_t);
	return(ninq);
}

/*
 * Compare creation times.
 */
static int
compar(p1, p2)
register struct quent **p1, **p2;
{
	if ((*p1)->q_ttime < (*p2)->q_ttime)
		return(-1);
	if ((*p1)->q_ttime > (*p2)->q_ttime)
		return(1);
	return(0);
}

/*
 * There is a problem about hogging a line with multiple transfers.
 * If done more than 50 transfers to a site in one transfer and there
 * are more than 20 more to do then close the connection and sleep
 * for a minute.
 */

getentry()
{
	register i;
	static  firsttime = 1;
	char *hnm;
	struct fent *fptr;
	struct hent *hptr;

#ifdef	FULLPP

	if (pp)
	{	char *host;
		char *net;
		strcpy(realname, "PP unspooled message");
		while(ppp_getnextmessage(&host, &net) == PP_OK)
		{	if (pp_connecttohost(host, net))	return 1;
			ppp_status (PPP_STATUS_CONNECT_FAILED, "");
		}
		L_LOG_0(L_MAJOR_COM, L_DATE|L_TIME, "Stopping as PP done\n");
		stat_close((char *) 0);
		ppp_terminate();
		exit(24);       /* Magic Number */
	}
#endif	/* FULLPP */

	/* First time through, build a queue from scratch */
	if(firsttime){
		firsttime = 0;
		hcount = 0;
		mailtrans = 0;
		i = buildq(qfdir, ANYHOST, b_name);
		if(i < 0)               /* some more to do later */
		{	L_LOG_0(L_MAJOR_COM, L_DATE|L_TIME, "Stopping as no work\n\n");
			stat_close((char *) 0);
			unlock_byte(lockname, lockfd, lockbyte);
			exit(24);       /* Magic Number */
		}
		else if(i == 0)         /* none at all */
		{	L_LOG_0(L_MAJOR_COM, L_DATE|L_TIME, "Stopping as queue empty\n\n");
			stat_close((char *) 0);
			unlock_byte(lockname, lockfd, lockbyte);
			exit(0);
		}
		goto got;
	}
again:

	/* If there is an active host, try to re-use it ... */
	if(net_open){
		/* Any still in the pending q ? Try them */
		if(curqent != NULL)
			while(++curqent < qend)
				if(strcmp((*curqent)->q_hname,b_name) == 0)
					goto nextent;

		/* Someone's changed something, so rethink */
		if(b_changed) b_sync();

		/* None in Q for this host -- maybe some more have appeared */
		if((i = buildq(qfdir, SAMEHOST, b_name)) > 0)
			goto got;

		/* None at all -- lets give up */
		if(i == 0)
		{	L_LOG_0(L_MAJOR_COM, L_DATE|L_TIME,
				"Stopping as queue now empty\n\n");
			stat_close((char *) 0);
			unlock_byte(lockname, lockfd, lockbyte);
			exit(0);        /* not got any at all */
		}

		/* no jobs available on this host */
		if(Curbp != NULL){
			Curbp->b_hc.h_nextattempt = 0;
			b_changed = 1;
		}
		con_close();
	}

	/* If anyone's touched something, let me know */
	if(b_changed) b_sync();

	/* Now look for another host */
	if((i = buildq(qfdir, OTHERHOST, b_name)) > 0)
		goto got;
	/* Or even the same host ! (e.g. one transfer per call) */
	else if((i = buildq(qfdir, ANYHOST, b_name)) > 0)
		goto got;

	/* OK -- nothing there */
	if(i == 0)
	{	L_LOG_0(L_MAJOR_COM, L_DATE|L_TIME, "Stopping as queue seems empty\n\n");
		stat_close((char *) 0);
		unlock_byte(lockname, lockfd, lockbyte);
		exit(0);                /* not got any at all */
	}
	else {	/* get here if we have got nothing except try the host */
		L_LOG_0(L_MAJOR_COM, L_DATE|L_TIME,
			"Stopping as there seems to be no more work\n\n");
		stat_close((char *) 0);
		unlock_byte(lockname, lockfd, lockbyte);
		exit(24);       /* Magic number time */
	}

got:
	curqent = qentrys;
nextent:
	/* Some hosts have a limited number of transfers per call.
	 * The others shouldn't be hogged for too long ...
	 */
	if(!net_open)
		hcount = 0;
	else if(++hcount >= maxftptrans || hcount >= NMAXTSTRANS){
		if(b_changed) b_sync();
		hcount = 0;
		con_close();
		/*
		 * quickly check to see if we already have a likely
		 * candidate
		 */
		while(++curqent < qend) /* yeuch */
			if( strcmp((*curqent)->q_hname, b_name))
				goto nextent;
		/* only sleep if cannot try another another first */
		if( buildq(qfdir, OTHERHOST, b_name) > 0 )
			goto got;
		sleep(20);
		goto again;
	}

	fptr = find_file((*curqent)->q_name, (*curqent)->q_inode, &qfd, 1, -1);
	/* open the q file */
	if (fptr) switch(fptr->f_state){
	case XDONESTATE:
		L_ERROR_0(L_ALWAYS, 0, "Old done state\n");
		printf("Old done state\n");
	case DONESTATE:
	case REJECTSTATE:
	case ABORTSTATE:
	case CANCELSTATE: fptr = 0;
	}
	if (!fptr || !(fptr->f_valid & F_valid))
	{	(void) close(qfd);
		if (!*b_name) strcpy(b_name, (*curqent)->q_hname);
		while(++curqent < qend) /* yeuch */
			if(!net_open || strcmp((*curqent)->q_hname,b_name)==0)
				goto nextent;
		goto again;
	}

	/* Hmmm ... if last call was to a different service, look again */
	if(net_open && mailtrans != (tab.t_flags & T_TYPE)){
		(void) close(qfd);
		while(++curqent < qend)         /* yeuch */
			if( strcmp((*curqent)->q_hname,b_name) == 0)
				goto nextent;
		/*
		 * we could get in an infinate loop here so we close the
		 * connection. This is very unlikely to happen so it is not
		 * very expensive.
		 */
		if(b_changed) b_sync();
		con_close();
		goto again;
	}

	mailtrans = (tab.t_flags & T_TYPE);
	(void) lseek(qfd, 0L, 0);	/* go back to the begining */
	(void) time(&time_read);
	if(tab.t_queued == 0)
		tab.t_queued = time_read;
	h_time = time_read;
	hnm = (char *)&tab + tab.l_hname;
	(void) strncpy(b_name, hnm, BFSIZE-1);

	 /* If transfers are ordered, then check to see if this host has
	  * been disabled in the mean time
	  */
	if (ordered &&
	     (hptr = find_host(hnm, 0, 1, 1)) &&
	     (hptr->h_valid & H_off_mask))
	{	L_LOG_3(L_NOTRETRY, 0,
			"%s disabled for %s as ordered=%d\n",
			(*curqent)->q_name, hnm, ordered);
		(void) close(qfd);
		goto again;
	}
	/* Check that we should be trying this one .... */
	if (tab.l_nextattmpt > time_read)
	{	L_LOG_3(L_NOTRETRY, 0,
			"%s not yet due (%d:%02d)\n",
			(*curqent)->q_name,
			(tab.l_nextattmpt - time_read) /60,
			(tab.l_nextattmpt - time_read) %60);
		b_changed = 1;
		(void) close(qfd);
		goto again;
	}


	/* Should we be processing this one ? */
	if(Curhp == NULL || strcmp(Curhp->host_alias, hnm)){
		struct hent *hptr = find_host(hnm, &Curhp, 1, -1);
		if (!Curhp || !hptr || hptr->h_valid & H_dont_call)
		{	Curhp = NULL;
			(void) close(qfd);
			if(net_open) con_close();
			goto again;
		}

		for(i = 0 ; i < Curhp->n_nets ; i++)
			if(Curhp->n_addrs[i].net_name)
				break;
		if(i >= Curhp->n_nets)
			i = 0;
		trans_data0 = Curhp->n_addrs[i].n_ftptrans;
		maxftptrans = TRANS_MAX_VAL(trans_data0);
		trans_wind  = TRANS_WIND_VAL(trans_data0);
		trans_pkts  = TRANS_PKTS_VAL(trans_data0);
		trans_call  = TRANS_CALL_VAL(trans_data0);
		trans_revc  = TRANS_REVC_VAL(trans_data0);
		trans_unixniftp=TRANS_UNIXNIFTP_VAL(trans_data0);
		trans_cug   = TRANS_CUG_VAL(trans_data0);
		trans_fcs   = (TRANS_NFCS_VAL(trans_data0)) ? -1 :
			      TRANS_FCS_VAL(trans_data0);
	}
	if (Curbp == NULL || strcmp(Curbp->b_host, b_name)){
		/* find bp in table */
		for(Curbp = Bfile.b_hosts; Curbp < Lastbp ; Curbp++)
			if(strcmp(Curbp->b_host, b_name) == 0)
				break;
		if(Curbp >= Lastbp)
			Curbp = NULL;
	}
	L_LOG_0(L_GENERAL, L_DATE | L_TIME, "Submitted at ");
	L_LOG_1(L_GENERAL, L_CONTINUE, "%s",ctime( (int *)&tab.t_queued));
	if (trans_wind || trans_pkts || trans_call || trans_revc ||
		trans_unixniftp || trans_cug || trans_fcs)
	{	L_LOG_4(L_GENERAL, 0,
			"wind=%d, pkt=%d/%d, call=%d",
			trans_wind, trans_pkts, 8 << trans_pkts, trans_call);
		L_LOG_4(L_GENERAL, L_CONTINUE, ", revc=%d, unixniftp=%d, cug=%02x%s\n",
			trans_revc, trans_unixniftp, trans_cug,
			(trans_fcs < 0) ? ", NFCS" : (trans_fcs > 0) ? ", FCS" : "");
	}

	if (trans_call == TRANS_CALL_NEVER)
	{
		L_LOG_1(L_GENERAL, 0, "Never call %s\n", hnm);
		(void) close(qfd);
		goto again;
	}
	/* Now let's try to lock it .....
	 * THIS IS TOO LATE !!!
	 * Someone else may already have had the lock open when I read the
	 * q entry ....
	 */
	if (!net_open)
	{	unlock_byte(lockname, lockfd, lockbyte);
		sprintf(lockname, "%s", hnm);
		lockfd = lock_byte(lockname, 0, &lockbyte);
		if (lockfd < 0)
		{	
			L_LOG_2(L_GENERAL, 0, "Failed to lock %s (%d)\n",
				lockname, errno);
			/* Set it to be re-tryable in a little while ... */
			if(Curbp == NULL)
				Curbp = newbp();
			Curbp->b_hc.h_nextattempt = time_read + 60;
			b_changed = 1;
			(void) close(qfd);
			goto again;
		}
		L_LOG_3(L_10, 0, "Locked %s (%d) at %d\n",
			lockname, lockfd, lockbyte);
	}
	else	L_LOG_3(L_10, 0, "Still locked %s (%d) at %d\n",
			lockname, lockfd, lockbyte);
	uid = tab.l_usr_id;              /* get the users uid/gid */
	gid = tab.l_grp_id;
	if(cur_user == NULL || cur_user->pw_uid != uid){
		/* look user up in password file */
		setpwent();
		if((cur_user = getpwuid(uid)) == NULL){
			(void) close(qfd);
			L_WARN_1(L_GENERAL, 0, "Cannot find user %d\n",uid);
			while(++curqent < qend)         /* yeuch */
				if(!net_open ||
					strcmp((*curqent)->q_hname,b_name)==0)
					 goto nextent;
			goto again;
		}
		endpwent();
	}

	if(stats != NULL){
#ifdef	UNDEF
		if(ISMAILS(tab.t_flags & T_TYPE)){ /* ??? */
			/* it is a mail transfer - no stats */
			fclose(stats);
			stats = NULL;
		}
		else
#endif
		{	char *date = ctime(&time_read);
			date[20] = '\0';
			fprintf(stats, "\n%s", date+8);
		}
	}
#ifdef JTMP
	if(ISJTMP(tab.t_flags & T_TYPE)){
		if(!tab.l_jtmpname){
			L_WARN_0(L_GENERAL, 0, "No jtmp info file\n");
			/* this is not a bug if sendig back a reply */
		}
		else
			(void) strcpy(jtmpfile, (char *)&tab+ tab.l_jtmpname);
	}
#endif JTMP
	tabentry++;
	return(1);
}

/* write out the hcontrol file */

b_sync()
{
	lockfile(b_fd);
	checkbf(0);
#ifdef  DEBUG
	if(!b_changed)
		printf("b_sync called wrongly\n");
#endif
	b_changed = 0;
	(void) lseek(b_fd, 0L, 0);
	(void) write(b_fd, (char *)&Bfile, sizeof(Bfile));
	checkbf(1);
	unlockfile(b_fd);
}

/*
 * Routine to rewrite queues entry and inform
 * the user of the outcome of the transfer.
 */

rwq(valu)
int     valu;
{
	long    l;
	char    text[500];              /* text message to user */
	char    htext[100];
	struct hent *hptr;
	char    *p,*q;
	char    *typ = 0;

#ifdef	FULLPP
	if (pp)
	{	int rc;
		switch(valu)
		{
		case TRYINGSTATE:
		case GOSTATE:
					return;
		case DONESTATE:		rc = PPP_STATUS_DONE;
					break;
		case ABORTSTATE:
		case FABORTSTATE:
		case REJECTSTATE:	rc = PPP_STATUS_PERMANENT_FAILURE;
					break;
		default:
					if (mtafailure)
						rc = PPP_STATUS_CONNECT_FAILED;
					else
					
						rc = PPP_STATUS_TRANSIENT_FAILURE;
					break;
		}
		if (rc == PPP_STATUS_DONE)
		        *text = '\0';
		else
		{	if(!ecode && !*reason)
				(void)strcpy(reason, "(reason not known)");
			sprintf(text, "%s%s%s",
				(ecode ? ermsg[ecode-1] : ""),
				(ecode ? "\n" : ""),
				reason);
		}
		ppp_status(rc, text);
		return;
	}
#endif	/* FULLPP */

	/* Special case: Sucking from remote site.
	 * If it WORKED, the re-queue it for another attempt
	 */
	if ((ISNEWS(tab.t_flags & T_TYPE) || ISMAIL(tab.t_flags & T_TYPE)) &&
		(st_of_tran == (TERMINATED | TERMINATED_OK)) &&
		(f_access & ACC_GET) &&
		valu == DONESTATE)
			valu = REQSTATE;

	/* stop retries after a certain number of atempts */
	if(valu == REQSTATE || valu == TIMEOUTSTATE)
	{	if(time_read-tab.t_queued > Qtimeout ||
						tab.l_nretries > Qretries)
		{	valu = REJECTSTATE;
			if(!*reason) sprintf(reason,
		"The NIFTP process gave up after %d attempts over %ld hours",
					tab.l_nretries,
					(time_read-tab.t_queued) / 3600L);
			typ = "Timeout";
		}
		tab.l_nretries++;
	}

	if(valu == FABORTSTATE)
		tab.status = ABORTSTATE;
	else
		tab.status = valu;

	if (curqent && *curqent)
	{
		struct fent *fptr = find_file((*curqent)->q_name,
			(*curqent)->q_inode, 0, 0, 0);
		if (fptr)
		{	fptr->f_state = tab.status;
			fptr->f_next_try = tab.l_nextattmpt;
		}
		if (ordered && (tab.l_nextattmpt > time_read) &&
			(hptr=find_host((char *)&tab + tab.l_hname, 0, 1, 1)))
		{	L_LOG_4(L_10, 0, "disable %s as ordered=%d (%x > %x)\n",
				(char *)&tab + tab.l_hname,
				ordered,
				tab.l_nextattmpt, time_read);
			hptr->h_valid |= H_waiting;
		}
	}
	if(tabentry &&write(qfd,(char *)&tab,STRUCTTABSIZE) != STRUCTTABSIZE){
		L_WARN_2(L_GENERAL, 0, "Cannot rewrite queue (%d/%d)\n",
			qfd, errno);
		stat_close((char *) 0);
		exit(1);
	}
	switch(valu){
	case DONESTATE:
		tabentry=0;
		(void) time((int *)&l);
		if(ftp_print & L_GENERAL){
			if(l == time_read) l++;
			if(data_end == data_start) data_end++;
			L_ACCNT_3(L_GENERAL, L_TIME,
			"%ld bytes Rate=%ld bits/s Data=%ld bits/s\n",
				bcount,
				(bcount/(l - time_read)) << 3,
				(bcount/(data_end - data_start)) << 3);
		}
		if(stats != NULL){
			fprintf(stats, "s=%ld r=%ld ",
					bcount,(bcount/(l-time_read))<<3);
			fprintf(stats, "t=%ld z=g",l-time_read);
		}
#ifdef JTMP
		if(ISJTMP(tab.t_flags & T_TYPE))
			jtmp_fin();
		else
#endif JTMP
		/* ----- This should be done as the specified user ----- */
		/* ----- This should be done as the specified user ----- */
		/* ----- This should be done as the specified user ----- */
		/* ----- This should be done if it is SENDING      ----- */
		/* ----- This should be done if it is SENDING      ----- */
		/* ----- This should be done if it is SENDING      ----- */
		if((tab.t_flags & WRITE_DELETE) && *realname &&
			(tab.t_access & ACC_GET) == 0
			/*direction == TRANSMIT*/)
		{	L_LOG_2(L_GENERAL, 0, "Unlink %s%s\n", realname,
				(direction == TRANSMIT) ? "" :
				" ***** was about to leave it !!");
			unlink_realname(realname);
		}
		/* notify user if requested */
		if (tab.t_flags & NOTIFY_SUCCESS){
			if(cur_user == NULL){
			L_WARN_1(L_GENERAL, 0, "Unknown user %d\n",uid);
				break;
			}
			q = (char *)&tab + tab.l_hname;
			sprintf(text,f_ok_text,realname,tofrom,q,network);
			ni_rtn(cur_user->pw_name,f_ok_sub,text,"");
		}
		break;
	case REJECTSTATE:
	case ABORTSTATE:
	case FABORTSTATE:
		/* notify user */
		tabentry=0;
		if(stats != NULL)
			fprintf(stats, "z=%c",(valu==REJECTSTATE) ? 'r':'a');
#ifdef	MAIL
		if(ISMAILS(tab.t_flags & T_TYPE)){
		    if (!(f_access & ACC_GET)) 
		    {	if(!tab.l_from){
				if(cur_user == NULL){
					L_WARN_0(L_GENERAL, 0, "Can't find sender of mail\n");
					break;
				}
				p = cur_user->pw_name;
			}
			else
				p = (char *)&tab + tab.l_from;
			q = (char *)&tab + tab.l_hname;
			if(!ecode && !*reason)
				(void)strcpy(reason, "(reason not known)");
			sprintf(text,m_f_text,q,network,whichhost,
					(ecode?ermsg[ecode-1]:""),reason);
			if(typ == 0){
				if(valu == REJECTSTATE)
					typ = "Rejected";
				else
					typ = "Aborted";
			}
			sprintf(htext, m_f_sub, q,  typ);
			if ((!(*errfile) ||
			    log_in_file(p,htext,text,realname,errfile) != NI_RTN_OK) &&
			    ni_rtn(p, htext, text, realname) == NI_RTN_OK ||
			    ni_rtn("postmaster", htext, text, realname) == NI_RTN_OK)
				unlink_realname(realname);
			else	L_LOG_4(L_ALWAYS, 0,
					"*** file left in %s (%s) (%s/%s)\n",
					realname, localname, htext, text);
		    }
		}
		else
#endif	MAIL
#ifdef JTMP
		if(ISJTMP(tab.t_flags & T_TYPE)){
		    if (!(f_access & ACC_GET)) 
		    {	if(!tab.l_from){
				if(cur_user == NULL){
					L_WARN_0(L_GENERAL, 0, "Can't find sender of job\n");
					break;
				}
				p = cur_user->pw_name;
			}
			else
				p = (char *)&tab + tab.l_from;
			q = (char *)&tab + tab.l_hname;
			if(!ecode && !*reason)
				(void)strcpy(reason, "(reason not known)");
			sprintf(text,j_f_text,q,network,whichhost,
					(ecode?ermsg[ecode-1]:""),reason);
			if (ni_rtn(p,j_f_sub,text,"") == NI_RTN_OK || 
			    ni_rtn("postmaster",j_f_sub,text,"") == NI_RTN_OK)
				(void) unlink_realname(realname);
			else	L_LOG_4(L_ALWAYS, 0,
					"*** file left in %s (%s) (%s/%s)\n",
					realname, localname, j_f_sub, text);
			if(*jtmpfile){
				(void) unlink(jtmpfile);
				*jtmpfile = 0;
			}
		    }
		}
		else
#endif JTMP
#ifdef NEWS
		if(ISNEWS(tab.t_flags & T_TYPE)){
		    if (!(f_access & ACC_GET)) 
		    {	if(!tab.l_from){
				if(cur_user == NULL){
					L_WARN_0(L_GENERAL, 0, "Can't find news sender\n");
					break;
				}
				p = cur_user->pw_name;
			}
			else
				p = (char *)&tab + tab.l_from;
			q = (char *)&tab + tab.l_hname;
			if(!ecode && !*reason)
				(void)strcpy(reason, "(reason not known)");
			sprintf(text,n_f_text,realname,q,network,whichhost,
					(ecode?ermsg[ecode-1]:""),reason);
			if ((tab.t_flags & WRITE_DELETE))
			{	char	newname[ENOUGH];
				char	*slash = rindex(realname, '/');

				if (slash) sprintf(newname, "%*.*s/.TFTP.%s",
					slash-realname,
					slash-realname,
					realname,
					slash+1);
				else	sprintf(newname, ".TFTP.%s",realname);

				if (rename(realname, newname) < 0)
				{ struct stat statbuf;
				  if (stat(realname, &statbuf))
					L_LOG_4(L_GENERAL, 0,
					"no file %s (%s) (%s||%s) ?\n",
					realname, localname, n_f_sub, text);
				  else	L_LOG_4(L_ALWAYS, 0,
					"*** file left in %s (%s) (%s||%s)\n",
					realname, localname, n_f_sub, text);
				}
				else if ((!(*errfile) ||
			      		log_in_file(p,n_f_sub,text,"",errfile) != NI_RTN_OK) &&
					ni_rtn(p,n_f_sub,text,"") != NI_RTN_OK &&
			    		ni_rtn("postmaster",n_f_sub,text,"") != NI_RTN_OK)
					L_LOG_3(L_ALWAYS, 0,
					"*** Unprocessed file left in %s (%s/%s)\n",
					newname, n_f_sub, text);
			}
			if ((!(*errfile) ||
			    log_in_file(p,n_f_sub,text,"",errfile) != NI_RTN_OK) &&
			    ni_rtn(p,n_f_sub,text,"") != NI_RTN_OK &&
			    ni_rtn("postmaster",n_f_sub,text,"") != NI_RTN_OK)
			{	struct stat statbuf;
				if (stat(realname, &statbuf))
					L_LOG_4(L_GENERAL, 0,
					"no file %s (%s) (%s||%s) ?\n",
					realname, localname, n_f_sub, text);
				else	L_LOG_4(L_ALWAYS, 0,
				     "*** file left in %s (%s) (%s||%s)\n",
					realname, localname, n_f_sub, text);
			}
			/* Leave file for manual intervention */
		    }
		}
		else
#endif NEWS
		{
			if(cur_user == NULL){
				L_WARN_1(L_GENERAL, 0, "Unknown user %d\n",uid);
				break;
			}
			if(!ecode && !*reason)
				(void)strcpy(reason, "(reason not known)");
			q = (char *)&tab + tab.l_hname;
			sprintf(text,f_f_text,(char *)&tab + tab.r_fil_n,
				tofrom,q,network,whichhost,
				(ecode?ermsg[ecode-1]:""),reason);
			if(typ == 0){
				if(valu == REJECTSTATE)
					typ = "Rejected";
				else
					typ = "Aborted";
			}
			sprintf(htext, f_f_sub, tofrom, q, typ);
			if (ni_rtn(cur_user->pw_name, htext, text, "") != NI_RTN_OK)
				ni_rtn("postmaster", htext, text, "");
		}
		if(valu == ABORTSTATE && net_open)
			con_close();
		break;
	case TIMEOUTSTATE:
		tabentry=0;
		if(stats != NULL)
			fprintf(stats, "z=t");
		break;
	case REQSTATE:
		tabentry=0;
		if(stats != NULL)
			fprintf(stats, "z=q");
		break;
	case TRYINGSTATE:
	case GOSTATE:
		(void) lseek(qfd,0L,0); /* seek back to begining */
		return;
	}
	(void) close(qfd);              /* finished with this entry */
}

/*
 * Routine to send the STOP command. Sends state of transfer and an
 * information message. Should also report on failures on SFT ( doesn't )
 */

do_stop()
{
	register struct sftparams *p;
	register val    c,comm,qual;
	val     integ;
	char    t_buff[256];
	char    sbuf[500];

	L_LOG_0(L_MAJOR_COM, 0, "STOP\n");
	for(p = sfts ; p->attribute != (char) 0xFF ; p++){
		if(p->attribute == STOFTRAN){   /* send state of transfer */
			p->sflags |= TOSEND;
			p->squalifier = INTEGER|EQ;
			p->ivalue = st_of_tran;
		}
		else if(p->attribute == INFMESS){
			if(infomsg){            /* send a message */
				free(infomsg);
				infomsg = NULL;
			}
			if(ecode){
				if(*reason)
					(void) strcat(reason, MSGSEPSTR);
				(void) strcat(reason, ermsg[ecode-1]);
			}
			if(!*reason)
				continue;
			ecode=0;
			infomsg = malloc(strlen(reason) + 1);
			(void) strcpy(infomsg, reason);
			p->sflags |= TOSEND;
			p->squalifier = p->qualifier;
		}
	}
	send_qual(STOP);                /* send STOP */
#ifdef  UCL_ISID                /* isid code */
	if(p77flag)             /* we will not get STOPACK */
		c = 0xFF;       /* not STOPACK */
	else
#endif
	c = r_rec_byte();
	tstate = STOPACKs;              /* say waiting for STOPACK */
	if(c != STOPACK && (sftp(PROTOCOL)->ivalue & 0xFF00) == 0x0100){
		/* network failure + version 80 */
		/* blow the connection away.... Will retry from EOF */
		L_WARN_0(L_GENERAL, 0, "Read failure on waiting for STOPACK\n");
		killoff(0, REQSTATE, 0);
		/*NOTREACHED*/
	}
	tab.l_docket = 0;                /* delete the docket */
	may_resume =0;                  /* now */
		/* what to do if got an error here ???? */
	if(c!=STOPACK){
		L_WARN_0(L_GENERAL, 0, "Do we have a v77 system ??\n");
		if(net_open)
			con_close();
		return;
	}
	c = r_rec_byte();               /* number of attributes */
	L_LOG_0(L_MAJOR_COM, 0, "STOPACK with ");
	if (c)
	{	L_LOG_1(L_MAJOR_COM, L_CONTINUE, "%d parameters\n", c); }
	else	L_LOG_0(L_MAJOR_COM, L_CONTINUE, "no parameters\n");
	if(c & END_REC_BIT)
		return;
	sbuf[0] = 0;
	for(;c != 0; c--){
		comm = r_rec_byte();            /* read them in */
		if(comm & END_REC_BIT)          /* return if end of record */
			return;                 /* its an error but what */
		qual = r_rec_byte();            /* should be done ?? */
		if( qual & END_REC_BIT)
			return;
		dec_command(qual,t_buff,&integ);
		if(ftp_print & L_ATTRIB)
		{	L_LOG_0(L_ATTRIB, 0, "received ");
			log_attr(comm, qual, integ, t_buff);
		}
		switch(comm){                   /* print out what it is */
		case STOFTRAN:
			L_LOG_1(L_GENERAL, 0, "State of tran = %04x\n",integ);
			/* problems with other side */
			if( (st_of_tran & ~0xf) != (integ & ~0xf)){
				L_WARN_2(L_ALWAYS, 0,
			"Illegal state change on STOPACK from %04x->%04x\n",
					st_of_tran, integ);
				if(st_of_tran == TERMINATED)
					st_of_tran |= TERMINATED_MSG;
				break;
			}
			if(st_of_tran > integ &&
			   st_of_tran != (ABORTED|ABORTED_POSS)){
				L_WARN_2(L_ALWAYS, 0,
					"Invalid st_of_tran %04x (%04x)\n",
					integ, st_of_tran);
				break;
			}
			st_of_tran = integ;
			break;
		case ACTMESS:                   /* print out any messages */
			L_LOG_1(L_ALWAYS, 0, "Action message:- %s\n", t_buff);
			break;
		case INFMESS:
			L_LOG_1(L_ALWAYS, 0, "Information message:- %s\n",
					t_buff);
			if(!*sbuf)
				(void) strcpy(sbuf, t_buff);
			else {
				(void) strcat(sbuf, MSGSEPSTR);
				(void) strcat(sbuf, t_buff);
			}
			break;
		default:                /* what is this value doing here ? */
			L_WARN_1(L_GENERAL, 0, 
				"Unusual STOPACK command %04x\n", comm);
			break;
		}
	}
	if(r_rec_byte() != END_REC_BIT){
		L_WARN_0(L_GENERAL, 0, "Protocol error on stopack (eor)\n");
	}
	/*
	 * cope with systems that send st_of_tran after info strings
	 */
	if(*sbuf && st_of_tran != TERMINATED){
		if(!*reason)
			(void) strcpy(reason, sbuf);
		else {
			(void) strcat(reason, MSGSEPSTR);
			(void) strcat(reason, sbuf);
		}
	}
}

/* routine to send GO. Command has no parameters so do it explicitly */

send_go()
{
	L_LOG_0(L_MAJOR_COM, 0, "GO\n");
	init_rec();
	add_to_rec(GO);
	add_to_rec(0);
	end_rec();
	shove_it();     /* push the data */
}

/*
 * routine to process hang up signals. ( Got when system is dying )
 * rewrite the queue and docket as fast as possible. With the help
 * of the nice() call
 */

SIG_TYPE sig1()
{
	(void) signal(SIGHUP,SIG_IGN);  /* ignore any more signals */
	nice(-150);                     /* stop everything else */
	if(!may_resume)
		tab.l_docket = 0;
	writedocket();
	if(*localname && !may_resume)
		(void) unlink(localname);
	L_LOG_0(L_ALWAYS, L_DATE | L_TIME, "Got a hang up\n");
	stat_close((char *) 0);
	exit(1);
}

/* called if got an internal error. I.E. got a bad signal */

SIG_TYPE sigabort(i)
{	extern void _exit();
	L_LOG_1(L_ALWAYS, L_DATE|L_TIME, "signal %d\n", i);
	fflush(stdout);
	stat_close((char *) 0);
	for (i=0; i<30; i++) signal(i, _exit);
	L_LOG_0(L_ALWAYS, L_DATE|L_TIME, "Zappp..\n");
	/* Abort seems to hang, so lets do it MY way .... */
	{	char *a = (char *) -1;
		(void) signal(SIGSEGV,SIG_DFL);
		(void) signal(SIGBUS,SIG_DFL);
		*a = -1;
	}
#ifdef	SIGIOT
	(void) signal(SIGIOT,SIG_DFL);
#else	SIGIOT
	(void) signal(SIGILL,SIG_DFL);
#endif	SIGIOT
	L_LOG_0(L_ALWAYS, L_DATE|L_TIME, "Abort..\n");
	abort();
	L_LOG_0(L_ALWAYS, L_DATE|L_TIME, "Aborted\n");
	exit(-1);
}

/* code to deal with read errors on an sft return
 * this is because the gateway aborts the transfer on the
 * processing of the sft.
 */

readfail()
{
	long    oh_time;
	int     t;

	(void) time(&oh_time);
	/*
	 * try again later. Cannot
	 * try less than once / hour
	 */
	if(hcount == 1 && tstate == SFTs)       /* probarbly closed cos */
		t = 0;                          /* cannot do multiple */
	else {                                  /* transfers. */
		if(Curbp == NULL)
			Curbp = newbp();
		t = first_wait * ++Curbp->b_hc.h_nback;
		if(t > max_wait || t <= 0) t = max_wait;
	}
	if(Curbp)
	{	Curbp->b_hc.h_nextattempt = oh_time + t;
		b_changed = 1;
	}
	tab.l_nextattmpt = oh_time + t;
}

struct  backoff *
newbp()
{
	register struct backoff *bp, *np;
	static  struct  hcontrol zerohc = { 0 }; /* a null hcontrol */
	long    t;

	b_changed = 1;
	/* easy just get the next entry */
	if(Bfile.b_nhosts < MAXBHOSTS){
		Bfile.b_nhosts++;
		bp = Lastbp++;
		goto found;
	}
	/* get an entry which is no longer needed */
	(void) time(&t);
	for(bp = Bfile.b_hosts; bp < Lastbp ; bp++)
		if(bp->b_hc.h_nextattempt == 0 || bp->b_hc.h_nextattempt < t)
			goto found;
	/* finally just junk the last entry */
	for(t = 0, np = NULL, bp= Bfile.b_hosts ; bp < Lastbp ; bp++)
		if(t == 0)
			t = bp->b_hc.h_nextattempt;
		else
			if(bp->b_hc.h_nextattempt < t){
				np = bp;
				t = bp->b_hc.h_nextattempt;
			}
	if( (bp = np) == NULL)
		bp = Bfile.b_hosts;
found:;
	(void) strcpy(bp->b_host, b_name);
	bp->b_hc = zerohc;
	return(bp);
}

checkbf(when)
{
	static  long    ltime;
	struct  stat    statbuf;
	register struct backoff *bp, *xp;
	struct  bfile   Bf;

	(void) fstat(b_fd, &statbuf);
	if(!ltime || when){
		ltime = statbuf.st_mtime;
		return;
	}
	if(statbuf.st_mtime <= ltime)
		return;
	/*
	 * file has been modded recently.
	 * find out what has changed
	 *   Must read in old bfile. then see what differences have occured
	 *   then update our copy to reflect it.
	 */
	(void) lseek(b_fd, (long)0, 0);
	if(read(b_fd, (char *)&Bf, sizeof(Bf)) != sizeof(Bf)){
		L_WARN_1(L_GENERAL, 0, "Read failure on Bf - %d\n", errno);
		return;
	}
	for(xp = Bf.b_hosts ; xp < &Bf.b_hosts[MAXBHOSTS] ; xp++){
		if(!*xp->b_host)
			continue;
		for(bp = Bfile.b_hosts ; bp < Lastbp ; bp++){
			if(!*bp->b_host)
				continue;
			if(strcmp(bp->b_host, xp->b_host) == 0){
				/*
				 * oh heck. time to see if we have to
				 * do anything with this
				 */
				if(xp->b_hc.h_nextattempt != NEVER){
					/*
					 * is this a restart ?
					 */
					if(bp->b_hc.h_nextattempt != NEVER)
						break;
					/*
					 * ok. It it's a restart.
					 */
					bp->b_hc.h_nextattempt =
						       xp->b_hc.h_nextattempt;
					b_changed = 1;
				}
				else {
					bp->b_hc.h_nextattempt = NEVER;
					b_changed = 1;
				}
				break;
			}
		}
		if(bp >= Lastbp){
			/*
			 * oh hell. Found an entry in the Bfile
			 * which is not in our copy....
			 * patch our tables only if it is a disable
			 */
			if(xp->b_hc.h_nextattempt != NEVER)
				continue;
			bp = newbp();   /* get a new entry. */
			(void) strcpy(bp->b_host, xp->b_host);  /* fix name */
			bp->b_hc.h_nextattempt = NEVER;
		}
	}
}

/* Do an unlink, checking that it is a valid type first .... */
unlink_realname(realname)
char * realname;
{	char *reason = "stat failed";
	struct stat fileinf;
	int am_user = 0;
	int can_stat = 0;

	if (!stat(realname, &fileinf))	can_stat++;
	else
	{	am_user++;
		set_userids();
		if (!stat(realname, &fileinf))
		{	can_stat++;
			L_WARN_1(L_GENERAL, 0,
				"Had to become user to stat %s\n", realname);
		}
	}

	if (can_stat) switch (fileinf.st_mode & S_IFMT)
	{
	case S_IFDIR:	reason = "directory";	break;
	case S_IFCHR:	reason = "C special";	break;
	case S_IFBLK:	reason = "B special";	break;
	case S_IFREG:	reason = (char *) 0;	break;
	case S_IFLNK:	reason = (char *) 0;	break;
#ifdef	S_IFIFO
	case S_IFIFO:	reason = "FIFO";	break;
#endif	S_IFIFO
	default:	reason = "unknown type";break;
	}
	if (reason)
	{	L_ERROR_3(L_ALWAYS, 0,
			"Was about to unlink %s (%d %s)\n",
			realname, errno, reason);
	}
	else
	{	if (unlink(realname))
		{	L_ERROR_2(L_ALWAYS, 0,
				"Unlink %s failed %d\n", realname, errno);
		}
		L_WARN_1(L_GENERAL, 0, "delete %s\n", realname);
	}
	if (am_user) set_sysids();
}

#ifndef _42
/* rename a file, This is done as a routine for when 4.2 comes allong */

rename(f1,f2)
char    *f1,*f2;
{
	if (!link(f1,f2) || !unlink(f1)) return -1;
		
	return 0;
}
#endif

#ifdef NEWS
/*
 * send file to rnews
 * make certain file 0,1,2 are open when calling daemon
 *
 * This version waits for daemon to complete,
 * this could give us FTP timeouts, we shall see.
 */

do_news(file)
char    *file;
{
	register i, cpid;
	int     status;
	char newsbuf[ENOUGH];
#ifdef  VFORK
	while((i=vfork())==-1) sleep(1);
#else
	while((i=fork())==-1) sleep(1);
#endif
	if(!i){		/* child - start the news deamon */
		close(0);
		for(i = 2 ; i < NFILES ; i++)       /* close ring channels */
			close(i);
		dup(open("/dev/null",2));       /* Why not ..... */
		setuid(uid);
		setgid(gid);
		sprintf(newsbuf, NEWSproc, file);
		L_LOG_1(L_LOG_EXEC, 0, "news cmd = %s\n", newsbuf);
		execl("/bin/sh", "sh", "-c", newsbuf, 0);
		L_WARN_1(L_GENERAL, 0, "Cannot execute news daemon - %s\n",
							newsbuf);  /* eh ?? */
		exit(-1);
	}
	while((cpid = wait(&status))!= i && cpid != -1) continue; /* wait for him */
	if(status){                             /* decode status */
		L_WARN_1(L_GENERAL, 0, "Failed on daemon start %04x\n", status);
		st_of_tran = TERMINATED|TERMINATED_MSG;
		ecode = ER_NOT_PROCESSED;
	}

}
#endif NEWS


/* -- WORK NEEDED FOR FLOCK SYSTEMS -- see below -- */
lock_byte(name, max, pbyte)
char *name;
int *pbyte;
{	int fd = open(name, O_RDWR | O_CREAT, 0666);
	long byte;

	L_LOG_4(L_10, 0, "lock %s %d (%x) - %d\n", name, max, pbyte, fd);
	
	if (fd < 0) return fd;
	errno = -1;
	/* write(fd, some junk, max+1); */
	for (byte=0; byte<=max; byte++)
	{
#ifdef	FLOCK
#ifndef	FCNTL
		/* -- flock code will only ever allow ONE at a time
		 * -- Need to (e.g) append '".%d", byte' to filename
		 */
		if (byte > 0)
			L_LOG_2(L_GENERAL, 0,
				"It is pointless to lock %s with byte %d\n",
				name, byte);
#endif	FCNTL
#endif	FLOCK
		if (!lockfile3(fd, byte, 0))
		{
			if (pbyte) *pbyte = byte;
			return fd;
		}
	}
	L_LOG_3(L_10, 0, "lockfile %s (%d) failed %d\n", name, fd,errno);
	*name = '\0';
	close(fd);
	return -1;
}

unlock_byte(name, fd, byte)
char *name;
{
	L_LOG_3(L_10, 0, "Unlock %s %d %d\n", name, fd, byte);
	if (fd >= 0)
	{	unlockfile2(fd, byte);
		close(fd);
	}
	if (name && *name)
	{	unlink(name);
		L_LOG_1(L_10, 0, "unlink %s after unlocking\n", name);
		*name = '\0';
	}
}

test_lock(name)
char *name;
{	int fd = open(name, 2);
	long  byte = 0;
	int rc = 1;

	if (fd < 0)
	{	L_LOG_2(L_10, 0, "test_lock(%s) was OK as -1 (%d)\n",
			name, errno);
		return rc;
	}

	if (lockfile3(fd, byte, 0) == 0)
		unlockfile2(fd, byte);
	else	rc = -1;
	L_LOG_2(L_10, 0, "test_lock(%s) gave %d\n", name, rc);
	close(fd);
	return rc;
}

#ifdef	FULLPP

/* This has a standard "rc = read(fd, buff, len)" interface */
/* CHECK THIS VERY CAREFULLY ....
 * Who supplies the buffer ?
 * Pro tem, if PP does, bcopy it ....
 */
pp_read_it(fd, buff, len)
char *buff;
{	static 	char *buff2;
	static int count;

	L_LOG_3(L_10, 0, "pp_read_it(%d, %x, %d)\n", fd, buff, len);
	if (count) {
		len = len < count ? len : count;
		bcopy (buff2, buff, len);
		buff2 += len;
		count -= len;
	}
	else {
		if (ppp_getdata(&buff2, &count) == PP_NOTOK)
			len = -1;
		else {
			len = len < count ? len : count;
			if (len >= 0)
				bcopy(buff2, buff, len);
			count -= len;
			buff2 += len;
		}
	}
	L_DEBUG_1(L_10, 0, "read from PP %d bytes\n", len);
	if (len > 0 && ftp_print & L_SEND_NET) {
		fputs (">>> ", stdout);
		fwrite (buff, len, 1, stdout);
		fputs (" <<<\n", stdout);
	}
	return len;
}

/* This currently ignores the network info, as it is still being sorted out */
pp_connecttohost(host, network)
char *host;
char *network;
{	static id = 0;
	static struct tab std = { 0x0001, T_PP, 0, 0, 0,
			     0, 38,
			     0,0,0,
			     0,0,0,0,
			     0,0,0,0,0,
			     0,0,0,0,0,0,0,0,
			     "unset..."
			 };
	char *ptr;

	std.l_usr_id = PPuid;

	P_pp_spooled = 0;
	L_LOG_1(L_10, 0, "Try to connect to `%s'\n", host);
	tab.udocket.transfer_id = (getpid() << 8) + id++;
	bcopy(&std, &tab, sizeof std);
	tab.tptr = (char *) std.text - (char *) &std;
	ptr = (char *) &tab + tab.tptr;

	tab.l_hname = ptr - (char *) &tab;
	strcpy(ptr, host);
	while (*ptr++) continue;

	tab.l_fil_n = ptr - (char *) &tab;
	sprintf(ptr, "PP unspooled transfer to %s/%s", network, host);
	while (*ptr++) continue;

	tab.tptr = ptr - (char *) &tab;

	uid = tab.l_usr_id;              /* get the users uid/gid */
	gid = tab.l_grp_id;
	may_resume = 0;
	try_resuming = 0;
	mailtrans = (tab.t_flags & T_TYPE);
	L_LOG_4(L_10, 0, "access=%x, t_flags=%x, uid=%d, gid=%d, ",
		tab.t_access, tab.t_flags, uid, gid);
	L_LOG_3(L_10, L_CONTINUE, "id=%x, text=`%s', rest at %d\n",
		tab.l_docket, tab.text, tab.tptr);
	L_LOG_2(L_GENERAL, 0, "Try call to %s on %s\n", host, network);
	if(Curhp == NULL || !net_open || (--maxftptrans) == 0 ||
	   (strcmp(Curhp->host_alias, host) != 0 &&
	    strcmp(Curhp->host_name,host) != 0))
	{	int i;
		if(net_open)	con_close();
		L_LOG_2(L_10, 0, "setup info for %s on %s\n", host, network);
		Curhp = dbase_get(host);
		L_LOG_2(L_10, 0, "%s gave %x\n", host, Curhp);
		if(Curhp == NULL || Curhp->n_disabled){
			Curhp = NULL;
			L_LOG_0(L_GENERAL, 0, "Sigh\n");
			return 0;
		}

		for(i = 0 ; i < Curhp->n_nets ; i++)
			if(Curhp->n_addrs[i].net_name &&
			   same_net (network, Curhp->n_addrs[i].net_name, 1))
				break;
		if(i >= Curhp->n_nets) network = Curhp->n_addrs[i=0].net_name;
		trans_data0 = Curhp->n_addrs[i].n_ftptrans;
		L_LOG_4(L_10, 0, "select net %d (%s for %s) ftptrans=%x\n",
			i, Curhp->n_addrs[i].net_name, network, trans_data0);
		maxftptrans = TRANS_MAX_VAL(trans_data0);
		trans_wind  = TRANS_WIND_VAL(trans_data0);
		trans_pkts  = TRANS_PKTS_VAL(trans_data0);
		trans_call  = TRANS_CALL_VAL(trans_data0);
		trans_revc  = TRANS_REVC_VAL(trans_data0);
		trans_unixniftp=TRANS_UNIXNIFTP_VAL(trans_data0);
		trans_cug   = TRANS_CUG_VAL(trans_data0);
		trans_fcs   = (TRANS_NFCS_VAL(trans_data0)) ? -1 :
			      TRANS_FCS_VAL(trans_data0);
		L_LOG_0(L_10, 0, "Dummy tab entry created for PP xfer\n");
		tab.l_network = ptr - (char *) &tab;
		strcpy(ptr, network);
		while (*ptr++) continue;
		tab.tptr = ptr - (char *) &tab;
		return 1;
	}
	L_LOG_2(L_GENERAL, 0, "Call to %s on %s still open\n", host, network);
	return 1;
}
#endif	/* FULLPP */
