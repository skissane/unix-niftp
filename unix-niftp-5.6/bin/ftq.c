#ifndef	lint			/* unix-niftp bin/ftq.c */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/bin/RCS/ftq.c,v 5.6.1.6 1993/01/10 07:08:20 pb Rel $";
#endif	lint

/*
 * The Q Process Daemon
 *
 * $Log: ftq.c,v $
 * Revision 5.6.1.6  1993/01/10  07:08:20  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  16:59:39  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:31:25  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.3  89/07/16  13:59:55  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:35:36  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:05:08  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:08:18  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:23:44  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:25:14  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:02:39  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/07/12  09:35:55  pb
 * Convert to new logging
 * 
 * Revision 5.0  87/03/23  03:19:05  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "ftp.h"
#include "stat.h"
#include "files.h"
#include "nrs.h"
#include "infusr.h"
#include "../version.h"

#if	(defined(F_APPEND) && !defined(F_GETFL)) || defined(FCNTL)
#include <fcntl.h>
#endif

/*
 * ftq.c
 * last changed  6-Feb-86
 *
 */

char	version[] = VERSION;

extern  char    reason[];

static	char    logfile[ENOUGH];
static	char    statfile[ENOUGH];
static  char    *lsnaddress;
extern  char    ltranstype;
extern	sig_bits;
char    unknown_host;
char    *index(), *rindex();
long	time_start, time_stop;
long	data_start, data_stop;
long	mail_start;

/*
 * on a low level error, the low level routine calls reset which will pop
 * the stack and reenter main at the start of the main loop
 */

main(argc,argv)
char    **argv;
{
	register int    i,flag;
	static  char    _host[ENOUGH];
	static  char    _localname[ENOUGH];
	static  char    _realname[ENOUGH];

	localname = _localname;
	hostname = _host;
	realname = _realname;
	*localname=0;
	*realname=0;
	initprocess(argc,argv);         /* initialise the process */
	error =0;
	setjmp(rcall,0);
	for(;;){
		if(error){
			stat_state(S_ERRCLOSE);
			if(qfd != -1)         /* close sucker docket */
			{
				L_LOG_1(L_GENERAL, 0, "Close sucker docket (%d)\n",
					qfd);
				(void) close(qfd);
				qfd = -1;
			}
			if(f_fd != -1){         /* close local file */
				(void) close(f_fd);
				f_fd = -1;
			}
			if(may_resume)          /* write docket if can */
				writedocket();                /* resume */
			deldocket(may_resume);   /* otherwise delete it */
			error =0;
			tstate = 0;
			if(*localname && !may_resume){ /* delete  files */
				(void) unlink(localname);
			}
			*localname = 0;
			if(net_open)    /* close connection after an error */
				con_close();
			/* Started up by yb daemon -- so die ... */
			if (q_daemon)
			{	stat_close((char *) 0);
				exit(0);
			}
		}
		else if(direction != 0)
			goto retry;
		if(net_open)            /* close connection if open */
			goto another;
		fclose(stdout);
		if( (freopen(logfile,"a",stdout)) == NULL)
			freopen("/dev/null", "w", stdout);

#ifdef	FOPEN_A_APPENDS
#ifdef	F_GETFL
		else if ((i=fcntl(fileno(stdout), F_GETFL)) != -1)
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

		(void) strcpy(hostname, lsnaddress);
		L_LOG_1(L_FULL_ADDR, L_TIME, 
			"Opening connection to service %s\n", hostname);
		stat_state(S_LISTEN);
		if((i = con_listen(hostname)) != 0){
			stat_state(S_LISTENFAILED);
#ifdef  UCL
			if(i > 0 || i == -3){
				L_WARN_1(L_GENERAL, 0,
					"Unknown address %s\n",hostname);
				stat_close((char *) 0);
				exit(29); /* get out while the going is good*/
			}
			L_WARN_1(L_GENERAL, 0,
				    "Listen to %s failed\n", hostname);
			continue;
#else
			L_WARN_1(L_GENERAL, 0,
				"Listen to %s failed\n", hostname);
			stat_close((char *) 0);
			exit(29);
#endif
		}
another:	/* come here for another transfer */
		stat_name(hostname);
		stat_state(S_SFT);
		L_LOG_0(L_GENERAL, L_TIME, "Now for next transfer (or close)\n");
		time(&time_start);
		mail_start = 0;
		direction =0;
		flag = 0;

		set_defval();   /* set up default values */
		starttimer(2*60);
		get_sft();      /* read the sft and send the reply */
		writedocket();
		stat_val(0);
		stat_state(S_DATA);
		if(st_of_tran ==VIABLE && lastmark != INFINITY){
			L_LOG_0(L_MAJOR_COM, 0, "Waiting for GO\n");
			i = r_rec_byte();       /* wait for the GO */
			if(i != GO){
				if(i == STOP)   /* got a STOP */
					flag++;
				else {          /* else error */
					L_WARN_0(L_GENERAL, 0,
						    "bad return to SFT\n");
					error++;
					continue;
				}
			}
			else if(r_rec_byte()!=0|| r_rec_byte()!=END_REC_BIT){
				L_WARN_0(L_GENERAL, 0, "Bad GO\n");
				error++;        /* got parameters with GO */
				continue;       /* command */
			}
			else {
				if(f_access & ACC_GET){
					L_LOG_0(L_10, 0, "transmitting\n");
					direction = TRANSMIT;
				}
				else {
					L_LOG_0(L_10, 0, "receiving\n");
					direction = RECEIVE;
				}
				writedocket();
				tstate = GOs;   /* about to send/get data */
		retry:
				time(&data_start);
				if(direction == TRANSMIT)
					transmit();
				else
					receive();
				time(&data_stop);
			}
		}
		else if(st_of_tran == VIABLE && lastmark == INFINITY){
			st_of_tran = TERMINATED;  /* failed at STOP */
			tstate = OKs;           /* resume with no data phase*/
		}
		stat_state(S_DONE);
		writedocket();
		direction =0;                   /* stop spurious results */
		(void) close(f_fd);
		f_fd = -1;
		if(net_open){
			stat_state(S_AWAITSTOP);
			waitstop(flag);         /* wait for stop command */
			if(error)
				continue;
		}
		else {                         /* net closed during transfer*/
			L_WARN_0(L_GENERAL, 0,
				"network failure in transfer\n");
			error++;
			continue;
		}
		if(st_of_tran == TERMINATED){   /* finished OK finish off */
#ifdef JTMP
			/*
			 * call the jtmp queuer to deal with jtmp jobs
			 */
			stat_state(S_PROCESS);
			if(JTMPTRANS && !(f_access & ACC_GET))
				exec_dj();
			else
#endif JTMP
#ifdef  MAIL
			/* don't do these if got mail */
			if(mailer && !(f_access & ACC_GET)){
				;       /* nothing */
			}
			else
#endif
#ifdef NEWS
			if(NEWSTRANS && !(f_access & ACC_GET))
			{	L_LOG_2(L_GENERAL, 0, "type = %x, access=%x\n",
					ltranstype, f_access);
				do_news(realname);
			}
			else
#endif NEWS
			if(f_access == ACC_TJI){ /* put job into jobmill */
				L_LOG_0(L_MAJOR_COM, 0, "Take job input\n");
				start_job(realname);
			}
			else if(f_access == ACC_TJO){    /* send file to lp */
				L_LOG_0(L_MAJOR_COM, 0, "Take job output\n");
				job_to_lp(realname);
				(void) unlink(realname);
			}               /* read and delete */
			else if((f_access == ACC_RAR || f_access == ACC_DR) &&
				(qfd == -1))
			{	L_LOG_1(L_GENERAL, 0,
				    "Deleteing read file '%s'\n", realname);
				if(unlink(realname)){
					L_WARN_2(L_GENERAL, 0,
					    "Unlink of '%s' failed (%d)\n",
					    realname, errno);
					st_of_tran = TERMINATED|TERMINATED_MSG;
					ecode = ER_NOT_DELETED;
				}
			}
			if(qfd != -1)         /* close sucker docket */
			{	struct tab tab;
#define	STRUCTTABSIZE	sizeof (struct tab)
				lseek(qfd, 0, 0);
				read(qfd,(char *)&tab,STRUCTTABSIZE);
				L_LOG_4(L_GENERAL, 0,
				"Sucker open (%d) access = %x/%x, flags %x\n",
					qfd, f_access,
					tab.t_access, tab.t_flags);
				tab.status = DONESTATE;
				lseek(qfd, 0, 0);
				write(qfd,(char *)&tab,STRUCTTABSIZE);
				close(qfd);
				qfd = -1;

				if((tab.t_access & ACC_GET) == 0 &&
					realname)
				{	L_LOG_2(L_GENERAL, 0,
						"Rem access %x%s\n",
						f_access,
						((f_access != ACC_RAR) &&
						 (f_access != ACC_DR)) ?
						"" : " -- Asked to delete");
					if (tab.t_flags & WRITE_DELETE)
					{	L_LOG_2(L_GENERAL, 0,
						"S Unlink %s%s\n", realname,
						(direction == TRANSMIT) ? "" :
						" ***** was about to leave it !!");
						unlink_realname(realname);
					}
					else
					{	L_LOG_1(L_GENERAL, 0,
						"DONT Unlink %s\n", realname);
					}
				}
				/* notify user if requested */
				if (tab.t_flags & NOTIFY_SUCCESS){
					if(cur_user == NULL){
					L_WARN_1(L_GENERAL, 0, "Unknown user %d\n",uid);
					}
					else
					{	char text[ENOUGH];
						char *q = (char *)&tab + tab.l_hname;
						sprintf(text,f_ok_text,realname, "<- ?? ->",q, "[ ?? net ?? ]");
						ni_rtn(cur_user->pw_name,f_ok_sub,text,"");
					}
				}
			}
			stat_state(S_PROCESSED);
			if(ftp_print & L_GENERAL) {
				long ef_data_stop;

				time(&time_stop);
				/* sort out effective data end
				 * take mail actions into account
				 */
				if(mail_start)
					ef_data_stop = mail_start;
				else
					ef_data_stop = data_stop;
				if(time_stop == time_start) time_stop++;
				if(ef_data_stop == data_start) ef_data_stop++;
				L_ACCNT_3(L_GENERAL, L_TIME,
			"%ld bytes Rate=%ld bits/sec Data=%ld bits/sec\n",
				bcount,
				(bcount/(time_stop - time_start)) << 3,
				(bcount/(ef_data_stop - data_start)) << 3);
			}
		}
		if(st_of_tran != TERMINATED)   /* problem with transfer */
			L_LOG_1(L_GENERAL, 0, "state of transfer = %04x\n",
								st_of_tran);

		/* HACK !!!!!!!! <<<<<<<<<<<<<<<<<<<<<<<<<<<< HACK !! */
		/* CAMEL doesn't understand soft error codes !
		 * Here is a HACK to a) guess it's camel & b) make it soft
		 */
		if (st_of_tran == (REJECTED | REJECTED_POSS) && net_open &&
			horiztab && strcmp("X       ", horiztab) == 0)
		{	L_LOG_0(L_GENERAL,0,"Close as soft error to CAMEL\n");
			con_close();
		}
		/* HACK !!!!!!!! <<<<<<<<<<<<<<<<<<<<<<<<<<<< HACK !! */

		if(net_open)            /* if can send STOPACK */
			stopack();
		else {
			error++;
			if(*reason && st_of_tran != TERMINATED)
				L_WARN_2(L_ALWAYS, 0,
				    "(%s) Failed because: %s\n", uid, reason);
			continue;
		}
		tstate =0;
		if(!error)
			deldocket(0);    /* can only close the docket now */
	}
}

#ifdef JTMP
/*
 * if this is a jtmp transfer then we signal the spooler to process
 * what it can find in the spool directory
 */
exec_dj()
{
	int     pid;
	int     fd;

	if( (fd = open(NRSdspooler, 0)) >= 0){
		if(read(fd, (char *)&pid, sizeof(pid)) != sizeof(pid))
			pid = 0;
		(void) close(fd);
	}
	else
		pid = 0;
	if(pid == 0 || kill(pid, SIGEMT)) L_WARN_1(L_GENERAL, 0,
			"Cannot signal spooler to submit job\n");
}
#endif

/*
 * routine to initialise the process
 */

extern  char    *regimodes;
extern  char    lreject, *linfomsg;
char    lnetchar;               /* character to put in for reverse lookup */
char    lcomplain;              /* complain about bad addresses */
char	lcreject;		/* reject bad calling addresses */
char	lreverse;		/* reject on bad reverse lookup */
char	lallow_tji;		/* allow Take Job Input */
char	*xfer_type = (char *)0;	/* type of xfer -- used with NSAPs */

initprocess(argc,argv)
char    **argv;
{
	register i;
	char    *ctime();
	struct  LISTEN  *lp;
	struct  passwd *pw, *getpwnam();
	char	*listener;		/* which listener */
	char	*cmd = argv[0];

/* signal catching table */

	extern  SIGRET sig1(),sigerr();

	static  SIGRET (*funcs[NSIG])() = {
		sig1, SIG_IGN, SIG_IGN, sigerr,
		sigerr, sigerr, sigerr, sigerr,
		SIG_DFL, sigerr, sigerr, sigerr,
		sigerr, SIG_IGN, sig1,
		};

	(void) umask(0); /* set default mask to 0 to give rwrwrw for logs */
	/* */
	while (argc > 1 && argv[1][0] == '-') switch (argv[1][1])
	{
	case '1':	q_daemon = 1;
			argv++; argc--;			break;
	case 't':	if (argv[1][2] == '\0')
			{	argv++; argc--;
				xfer_type = argv[1];
			}
			else	xfer_type = argv[1] +2;
			argv++; argc--;			break;
	default:
		{	char fullname[1024];
			sprintf(fullname, "%s/log.ERROR", LOGDIR);
			freopen(fullname, "a+", stdout);
			printf("%s: Bad argument `%s'\n", cmd, argv[1]);
			exit(-1);
		}
	}
	if(argc<2){
		/* Hmmm ... guess what is wanted .. */
		q_daemon = 1;
		listener	= rindex(cmd, '/');
		if (!(listener++)) listener = cmd;
	}
	else	listener = argv[1];

	if(nrs_init() < 0){
		char fullname[1024];
		sprintf(fullname, "%s/log.ERROR", LOGDIR);
		freopen(fullname, "a+", stdout);
		printf("Cannot initialise\n");
		exit(-1);
	}
	for(lp = LISTENS ; lp->Lname ; lp++)
		if(strcmp(lp->Lname, listener) == 0)
			break;
	if(lp->Lname == 0){
		char fullname[1024];
		sprintf(fullname, "%s/log.ERROR", LOGDIR);
		freopen(fullname, "a+", stdout);
		printf("Cannot find listener channel %s\n", listener);
		if (q_daemon) printf("(Rename %s as listener ??)\n", cmd);
		exit(1);
	}
	ftp_print = lp->Llevel;

	/* should really be in nrsinit */
	if(pw = getpwnam(FTPuser)) FTPuid = pw->pw_uid;

	for(i = 0 ; i < NSIG;i++)       /* set signal values */
		(void) signal(i+1,funcs[i]);
#ifdef  SIGXFSZ
	(void) signal(SIGXFSZ, SIG_IGN);
#endif
	for(i = 0 ; i < NFILES;i++)     /* close all files */
	    if (!q_daemon && i != 4)	/* the socket fd from the yb daemon */
		(void) close(i);        /* it's a daemon isn't it */

	freopen("/dev/null","r",stdin); /* get a stdin.fd 0 */

	/* get log file -- default is log.q<name> */
	if(lp->Llogfile == NULL)
		(void) sprintf(logfile, "%s/%s%s", LOGDIR, QLOG, listener);
	else if(*lp->Llogfile != '/')
		(void) sprintf(logfile, "%s/%s", LOGDIR, lp->Llogfile);
	else	(void) strcpy(logfile, lp->Llogfile);

	fclose(stdout);
	if( (freopen(logfile,"a",stdout)) == NULL)
		freopen("/dev/null","w",stdout);
	fclose(stderr);
	(void) freopen("/dev/null", "w", stderr);

#ifdef  SETLINEBUF
	setlinebuf(stdout);     /* line buffering for BSD */
#else	/* SETLINEBUF */
#ifdef  SETVBUF
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);     /* SYS V */
#else	/* SETVBUF */
	setbuf(stdout, NULL);
#endif	/* SETVBUF */
#endif	/* SETLINEBUF */

	/* get stat file -- default is ST.q<name> */
	if(lp->Lstatfile == NULL)
		(void) sprintf(statfile, "%s/%s%s", LOGDIR, QSTT, listener);
	else if(*lp->Lstatfile != '/')
		(void) sprintf(statfile, "%s/%s", LOGDIR, lp->Lstatfile);
	else	(void) strcpy(statfile, lp->Lstatfile);
	
	stat_init(statfile);

	if(lp->Laddress == NULL){
		if (!q_daemon)
		{	L_LOG_3(L_GENERAL, L_DATE | L_TIME,
				"Daemon %s %s Pid %d -- no listen string\n",
				cmd, version, getpid());
			exit(-1);
		}	
		lp->Laddress = "DAEMON";
	}
	lsnaddress = lp->Laddress;
	stat_serv(lp->Laddress);

	argstring = lp->Lchannel;
	L_LOG_3(L_GENERAL, L_DATE | L_TIME,
		"Daemon %s %s Pid %d started\n",
			cmd, version, getpid());
	L_LOG_2(L_GENERAL, 0, "monitoring level set to %d (%04x)\n",
		ftp_print, ftp_print);
	linfomsg	= lp->Linfomsg;
	lreject		= (lp->Lopts & L_REJECT)	!= 0;
	lcomplain	= (lp->Lopts & L_COMPLAIN)	!= 0;
	lcreject	= (lp->Lopts & L_CREJECT)	!= 0;
	lreverse	= (lp->Lopts & L_REVERSE)	!= 0;
	lallow_tji	= (lp->Lopts & L_ALLOW_TJI)	!= 0;
	if (xfer_type)
	{	L_LOG_1(L_GENERAL, 0, "xfer_type=%s\n", xfer_type);
	}
	if (linfomsg)
	{	L_LOG_1(L_GENERAL, 0, "InfoMsg=%s\n", linfomsg);
	}
	if (lp->Lopts)
	{	L_LOG_4(L_GENERAL, 0,
			"opts %x -> rej=%d, complain=%d, crej=%d, ",
			lp->Lopts, lreject, lcomplain, lcreject);
		L_LOG_2(L_GENERAL, L_CONTINUE,
			"rev=%d, tji=%d\n",
			lreverse, lallow_tji);
	}


	/*
	 * if nchar not set use first char of the network name
	 */
	if (!(lnetchar = lp->Lnchar)) lnetchar = *lp->Lname;
}

/* wait for the STOP command */

waitstop(flag)
int     flag;
{
	val     valu,comm,n,q;
	val     ip;
	char    t_buff[256];

	if(flag == 1)                   /* set if we got a STOP when we */
		comm = STOP;            /* were waiting for the GO command */
	else
		comm = r_rec_byte();
	if(comm != STOP){
		L_WARN_0(L_GENERAL, 0, "Bad STOP\n");
		prot_err(0,0,0);
	}
	valu = r_rec_byte();            /* number of parameters on command */
	L_LOG_1(L_MAJOR_COM, 0, "%d parameters on STOP\n",valu);
/*
 * A problem arrises when there is an end of record while reading in the
 * STOP. We could very well hang if we get this
 */
	if(valu & END_REC_BIT){
		L_WARN_0(L_GENERAL, 0, "Protocol error in stop command !!\n");
		error++;
		return;
	}
	else if(valu == 0){
		if(r_rec_byte() != END_REC_BIT){
			L_WARN_0(L_GENERAL, 0, "Protocol error in stop\n");
			if(net_open)
				con_close();
			error++;
		}
		return;
	}
	do {
		n = r_rec_byte();       /* get attribute */
		if(n&END_REC_BIT){
			valu = n;
			break;
		}
		q = r_rec_byte();       /* qualifier */
		if(q&END_REC_BIT){
			valu = q;
			break;
		}
		dec_command(q,t_buff,&ip);      /* get rest of command */
		switch(n){                      /* print values */
		case STOFTRAN:                  /* could be done better */
			L_LOG_1(L_GENERAL, 0, "state of transfer = %04x ",ip);
			switch(ip & ~0xF){
			case VIABLE:
				L_LOG_0(L_GENERAL, L_CONTINUE, "Viable (in error ??)\n");
				break;
			case REJECTED:
				L_LOG_0(L_GENERAL, L_CONTINUE, "Rejected\n");
				break;
			case TERMINATED:
				if(tstate== FAIL)
					ip |= 01;
				L_LOG_0(L_GENERAL, L_CONTINUE, "Terminated\n");
				break;
			case ABORTED:
				L_LOG_0(L_GENERAL, L_CONTINUE, "Aborted\n");
				break;
			default:
				L_LOG_1(L_GENERAL, L_CONTINUE, "Unknown %04x\n", n);
				st_of_tran = TERMINATED|TERMINATED_MSG;
				continue;
			}
			if(ip < st_of_tran
				   && (st_of_tran != (ABORTED|ABORTED_POSS) &&
					ip != (ABORTED|ABORTED_IMPOSS))){
				L_WARN_2(L_GENERAL, 0,
					"Bad returned st_of_tran %x < %x\n",
							     ip,st_of_tran);
			}
			else if(ip == (ABORTED|ABORTED_POSS) &&
				(st_of_tran&~0xF) == ABORTED){
				L_WARN_2(L_GENERAL, 0,
				"aborted st_of_tran %x (from %x)\n",
							ip, st_of_tran);
			}
			else
				st_of_tran = ip;
			break;
	case ACTMESS:
			L_LOG_1(L_ALWAYS, 0, "Action message is:- %s\n",t_buff);
			break;
	case INFMESS:
			L_LOG_1(L_ALWAYS, 0, "Information message:- %s\n",t_buff);
			break;
	default:
			/* got a STOP from SFT */
			if(flag && (ftp_print & L_GENERAL))
			{    L_WARN_2(L_GENERAL, 0,
			     "Got a problem with attrib %04x = %04x\n",n,q);
			}
			else if(!flag) L_WARN_2(L_MAJOR_COM, 0,
			  "Other Qualifier on STOP command %04x - %04x\n",n,q);
			break;
		}
	} while(--valu);        /* End of Do loop */

	if(valu == 0 && r_rec_byte() == END_REC_BIT)    /* the ok state */
		return;
	L_WARN_0(L_GENERAL, 0, "Protocol error in stop command !!\n");
	/*
	 * I must close the connection otherwise I will be out of step
	 */
	error++;
	if(net_open)
		con_close();
}

/* send a stopack with a state of transfer */

stopack()
{
	register struct sftparams       *p;

	L_LOG_0(L_MAJOR_COM, 0, "STOPACK\n");         /* construct it here */
	for(p = sfts; p->attribute != (char)0xFF ; p++){
		p->sflags &= ~TOSEND;
		if(p->attribute == STOFTRAN){
			p->ivalue = st_of_tran;
			p->squalifier = p->qualifier;
			p->sflags |= TOSEND;
		}
		else if(p->attribute == INFMESS){
			if(ecode){       /* give a reason if one available */
				if(*reason)
					(void) strcat(reason, MSGSEPSTR);
				(void) strcat(reason, ermsg[ecode-1]);
			}
			if(!*reason)
				continue;
			ecode =0;
			if(infomsg)
				free(infomsg);
			infomsg = malloc(strlen(reason) + 1);
			(void) strcpy(infomsg, reason);
			p->squalifier = p->qualifier;
			p->sflags |= TOSEND;
		}
	}
	send_qual(STOPACK);     /* send it here */
}

/*
 * send file to the line printer.
 * make certain file 0,1,2 are open when calling daemon
 */

job_to_lp(file)
char    *file;
{
	register i;
	int     status;
	char lprbuf[ENOUGH];
#ifdef  VFORK
	while((i=vfork())==-1) sleep(1);
#else
	while((i=fork())==-1) sleep(1);
#endif
	if(!i){         /* child - start the printer deamon */
		close(0);
		for(i = 2 ; i < NFILES ; i++)       /* close ring channels */
			close(i);
		dup(open("/dev/null",2));       /* stupid printer */
		setuid(uid);
		setgid(gid); /* lp will only delay for a few seconds at most*/
		if(devtypq && *devtypq == '-')
			sprintf(lprbuf, printer, devtypq, file, (username) ? username : "", devtypq, file);
		else
			sprintf(lprbuf, printer, "", file, (username) ? username : "", "", file);
		L_LOG_1(L_LOG_EXEC, 0, "lpr cmd = %s\n", lprbuf);
		execl("/bin/sh", "sh", "-c", lprbuf, 0);
		L_WARN_1(L_GENERAL, 0,
			"Cannot execute line printer - %s\n", lprbuf);/*eh??*/
		exit(-1);
	}
	while(wait(&status)!= i);               /* wait for him */
	if(status){                             /* decode status */
		L_WARN_1(L_GENERAL, 0, "Failed on lpr start %04x\n", status);
		st_of_tran = TERMINATED|TERMINATED_MSG;
		ecode = ER_NOT_PRINTED;
	}

}

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
	register i;
	int     status;
	char newsbuf[ENOUGH];
#ifdef  VFORK
	while((i=vfork())==-1) sleep (1);
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
		sprintf(newsbuf, NEWSproc, file, hostname, file);
		L_LOG_1(L_LOG_EXEC, 0, "news cmd = %s\n", newsbuf);
		execl("/bin/sh", "sh", "-c", newsbuf, 0);
		L_WARN_1(L_GENERAL, 0, "Cannot execute news daemon - %s\n",
							newsbuf);  /* eh ?? */
		exit(-1);
	}
	while(wait(&status)!= i);               /* wait for him */
	if(status){                             /* decode status */
		L_WARN_1(L_GENERAL, 0, "Failed on daemon start %04x\n", status);
		st_of_tran = TERMINATED|TERMINATED_MSG;
		ecode = ER_NOT_PROCESSED;
	}

}
#endif NEWS

/* this routine is null on the q side since it is not needed... */

readfail()
{
}

/* catch various signals */

SIGRET sig1()          /* hangup */
{
	(void) signal(SIGHUP,SIG_IGN);  /* stop any more signals */
	(void) nice(-150);              /* speed me up a lot */
	if(!may_resume)                 /* otherwise delete it */
		deldocket(0);
	else writedocket();              /* write out my docket */
	L_WARN_0(L_ALWAYS, L_TIME, "Got a hangup signal\n");
	stat_close((char *) 0);
	exit(1);
}

SIGRET sigerr(sig)             /* any other */
{
	L_WARN_2(L_ALWAYS, L_TIME, "Got a bad signal %d !!! %04x\n",
		sig, sig_bits);
	if ((sig_bits & 0x8f) != 0x8a)
	{	stat_close((char *) 0);
		exit(33);
	}
	L_WARN_0(L_ALWAYS, 0, "So keep going I guess ....\n");
}

/* Do an unlink, checking that it is a valid type first .... */
unlink_realname(realname)
char * realname;
{	char *reason = "stat failed";
	struct stat fileinf;
	if (!stat(realname, &fileinf))
		switch (fileinf.st_mode & S_IFMT)
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
	{	L_ERROR_2(L_ALWAYS, 0,
			"Was about to unlink %s (%s)\n",
			realname, reason);
		return -1;
	}
	else	if (unlink(realname)) /* delete the file */
	{	L_WARN_2(L_GENERAL, 0,
		    "Unlink_realname of '%s' failed (%d)\n",
		    realname, errno);
		st_of_tran = TERMINATED|TERMINATED_MSG;
		ecode = ER_NOT_DELETED;
		return -1;
	}
	else return 0;
}
