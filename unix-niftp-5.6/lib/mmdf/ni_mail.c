/* unix-niftp lib/mmdf/ni_mail.c $Revision: 5.6 $ $Date: 1991/06/07 17:01:15 $ */

#include "ftp.h"
#include "nrs.h"
#include "util.h"
#include "mmdf.h"
#include "conf_niftp.h"
#include <stdio.h>

extern  char    nicdfldir[];    /* Default directory for NIFTP channels */
extern  char    nichan[];
extern  char    reason[];

/* the mail interface q end - used to interface to the mmdf software
 * it changes its arguments around then calls the actual interface routines
 * This is not very good code. I just made it work.
 *
 * $Log: ni_mail.c,v $
 * Revision 5.6  1991/06/07  17:01:15  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:36:21  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:48:46  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:45:44  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */


extern  char    xerr;

char    *rp_valstr();
extern  errno;
#include <errno.h>

do_mail(file)
char *file;
{
	int     i;
	char    *astring = argstring;

	xerr = 0;
	if(astring == NULL){
		L_WARN_0(L_GENERAL, 0, "Null argstring to do_mail\n");
		astring = "unix";
	}
	if((i = ni_mail(file, astring, hostname, 2)) != 0){
		L_WARN_1(L_GENERAL, 0, "Mailer had a failure (%d)\n", i);
	}
	return(i | xerr);
}

/* variables to stop the child from hanging -
	Q) why does mmdf take so long ????
	A) ?!?!?!?!?!?!?!?!?!?!?!?!
*/

static  SIGRET     catchit();
static  SIGRET     (*saveit)();

#ifdef notdef
char  *fname;                   /* File to be transfered */
char  *queue;                   /* name of NIFTP queue          (sgate) */
char  *TSname;                  /* TS name of calling host */
int   max_tries;                /* Max number of retries */
				/* If 0 mailer will detach and */
				/* Not return */
char  **rpstring;               /* Where to stuff result string */
				/* This might be passed to the */
				/* remote NIFTP */
#endif

#define LINESIZE        80

#ifndef SOME
#define SOME            40
#endif

static  jmp_buf tout;   /* used to timeout the child */

ni_mail(fname, queue, TSname, max_tries)
char    *fname, *queue, *TSname;
int   max_tries;
{
	int      pid,status,fd;
	int      no_tries=0;            /* Number of tries to deliver */
	char     chpath[LINESIZE];      /* Full path of NIFTP channel */
	extern   Nichan *ni_srch[];
	int cpid;

	if(nichan[0] !='/')
		sprintf(chpath, "%s/%s", nicdfldir, nichan);
	else
		(void) strcpy(chpath, nichan);

/* now try several times to do it */

	do{
		while( (pid = fork()) ==-1) sleep(1);
		if(!pid){
			for(fd = 0; fd < 16; fd++)      /* close some files */
				close(fd);
			dup(open("/dev/null",2));   /* open standard i/o */
			setuid(MAILuid);	    /* so auth messages work*/
			execl(chpath, nichan, queue, fname, TSname, 0);
			exit(RP_MECH);                  /* help !! */
		}
		L_LOG_4(L_LOG_EXEC, 0, "execl( %s , %s , %s , %s",
					chpath,nichan,queue,fname);
		L_LOG_1(L_LOG_EXEC, L_CONTINUE, " , %s )\n",
					TSname);
		if(max_tries < 1){                    /* don't wait around */
			L_LOG_0(L_GENERAL, 0,
				"Not waiting for MMDF to transfer mail\n");
			return (OK);
		}
		switch(setjmp(tout)){
		case 0:
			/* set up the alarm */
			saveit = signal(SIGALRM, catchit);
			(void) alarm(300);      /* a period of time - 5 mins*/
			while((cpid = wait(&status)) != pid && cpid != -1) continue;
			/* if return is here then we have not timed out */
			(void) alarm(100);
			(void) signal(SIGALRM, saveit);
			break;
		default:
			/* we have timed out */
			L_WARN_0(L_GENERAL, 0, "Killing stuck child\n");
			(void) kill(pid, SIGTERM);
			sleep(10);
			(void) kill(pid, SIGKILL);
			while((cpid = wait(&status)) != pid && cpid != -1) continue;
			(void) alarm(100);
			(void) signal(SIGALRM, saveit);
			(void) strcpy(reason, "Mailer stuck - unknown error");
			L_WARN_1(L_GENERAL, 0, "%s\n", reason);
			return(NOTOK);
		}
		if(status & 0177){
			status &= MASK;
			L_WARN_0(L_ALWAYS, 0, "Mail process failed - with ");
			if(status & 0200)
			{	L_WARN_1(L_ALWAYS, L_CONTINUE,
					"a core dump (signal %d)\n",
							status & 0177);
			}
			else L_WARN_1(L_ALWAYS, L_CONTINUE,
					"signal %d\n", status & 0x7f);
			(void) strcpy(reason, "MMDF mailer crashed - retry");
			L_WARN_1(L_GENERAL, 0, "%s\n", reason);
			return(NOTOK);
		}
		status = (status >> 8) & MASK;
		if(!status){                    /* Fatal crash */
			(void) strcpy(reason, "MMDF mailer crashed");
			L_WARN_1(L_GENERAL, 0, "%s\n", reason);
			return(NOTOK);
		}
		if(rp_isgood(status)){          /* Success */
			L_LOG_0(L_GENERAL, 0,
				"Mail transferred correctly \n");
			return(OK);
		}
		if(rp_gbval(status) == RP_BNO){ /*Complete disaster give up*/
			char    *me;
			xerr = 1;
			switch(rp_gval(status)){ /* give a message */
			case RP_NDEL:
				me = "Illegal format JNT Mail file";
				break;
			case RP_NO:
				me = "General mail system failure";
				break;
			case RP_HUH:
				me = "Cannot parse message correctly";
				break;
			case RP_PARM:
				me = "Bad mail parameter";
				break;
			case RP_USER:
				me = "Unknown recipient(s)";
				break;
			default:
				me = "Fatal mail transfer";
				break;
			}
			sprintf(reason, "%s [%s]", me, rp_valstr(status));
			L_WARN_1(L_GENERAL, 0, "%s\n", reason);
			return(NOTOK);
		}
		if(access(fname, 0) < 0 && errno == ENOENT){
			char    *me = "mailer failure";
			sprintf(reason, "%s [%s]", me, rp_valstr(status));
			L_WARN_1(L_GENERAL, 0, "%s\n", reason);
			return(NOTOK);
		}
	}while(++no_tries < max_tries);         /* no more tries allowed */
/*
 * We have tried as many times as allowed
 */
	sprintf(reason,
		"Repeated temporary MMDF failure %d/%x after %d attempts [%s]",
				status, status, no_tries, rp_valstr(status));
	L_WARN_1(L_GENERAL, 0, "%s\n", reason);
	return(NOTOK);
}

/* time is up here */

static SIGRET
catchit()
{
	signal(SIGALRM,saveit);
	longjmp(tout, 1);
}
