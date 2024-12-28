/* unix-niftp bin/qft.c $Revision: 5.5 $ $Date: 90/08/01 13:32:07 $ */
/*
 ***********************************************************************
 * niftp     version 2    November 1984                                *
 * module:                                                             *
 *            ---------    qft.c        ---------                      *
 *      'queue a file transfer' user interface program                 *
 *     line at a time user interface program to queue niftp requests - *
 *      a bit like arpanet ftp                                         *
 *                                                                     *
 * by: Ruth Moulton                University College London           *
 * hacked to work with new ftp system           Phil. Cockcroft        *
 * last changed:   6-Nov-86                                            *
 ***********************************************************************
 */
#include "../version.h"
char	version[] = VERSION;
/* changes:
 * 12-may-82 rm  type 'c' in send or get to return to command mode instead
	      of 'q'
 * 24-oct-82 rm  allow embedded spaces in parameter values - for cms file
		 names for example - trailing spaces are stripped
 *  3-Dec-82 rm  define user by gid as well as uid
 * 11-mar-83 rm  use standard i/o library & use new tab.h
 * 18-apr-83 rm  take out references to unix
 * 17-jun-83 rm  add notify command + delete on write
 *  8-aug-83 rm  change for 2.8
 * 24-May-84 phil. Completely changed to work with new ftp system
 *  6-Nov-84 phil. Fix passwd buf.
 *  3-May-85 phil. Modify to work with NRS
 *  6-Nov-86 phil. Fix up local file handling for service users
 * $Log:	qft.c,v $
 * Revision 5.5  90/08/01  13:32:07  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:04:14  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:05:25  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:58:32  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:19:48  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

/* TO DO
shell commands
* and ? in local filenames
command to see all known host names
*/

#include "opts.h"
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sgtty.h>

#include "tab.h"
#include "nrs.h"
#include "files.h"

struct  tab     tab;    /* the entry for the queue */

#ifndef ENOUGH
#define ENOUGH  256
#endif

/* for SYSV etc ... */
#ifndef	TIOCSETN
#define	TIOCSETN	TIOCSETP
#endif	TIOCSETN

struct  passwd  *getpwuid();
struct  passwd  *cur_user;

char    ename[256];             /* effective name of queue file */
char    rname[256];             /* real name of queue file */

char    host_name[32];          /*Name of host to be looked up in configfile*/
char    channel[32];            /* name of channel */

char    queue[ENOUGH];          /* name of queue */

int     access_m;               /* access mode */
int     t_flags;                /* various flags */

char    devt[ENOUGH];           /* output device type */
char    devq[ENOUGH];           /* output device type qualifier */

int     queuefd;                /* the queue file descriptor */

char    cwd[ENOUGH];            /* current working directory */
char    realf[ENOUGH];          /* real name of file */
int     athome;                 /* set if user is in home directory */

extern  int     errno;

  /* command strings */
char    crhost[] = "host", cusr[] = "login-name",cpas[] = "password",
	cfilp[] = "file-password",cacc[] = "account",cacp[] = "acpassword",
	csend[] = "send",cget[] = "get", ccur[] = "current-values",
	clfiln[] = "local-filename",crfiln[] = "remote-filename",
	ctype[] = "type",
	csmode[] = "write-mode",cgmode[] = "read-mode",
	cnote[] = "notify" ,
	cwdel[] = "delete-local-file" ,
	cdev[] = "device-type",
	chelp[] = "help";

/* parameter strings */
char    ptia5[] = "ascii",ptbin[] = "binary",   /* file types */
	pgm1[] = "keep",pgm2[] = "delete",
	pgm3[] = "remove-during",/*get modes*/
	psm1[] = "make",psm2[] = "replace",psm3[] = "mor", /* send modes*/
	psm4[] = "append",psm5[] = "aom",
	pnot1[] = "ok",pnot2[] = "not-ok"; /* notify and delete-write cases*/

/* help texts: */
char    *help0[] = {
	"NOTE: - only first 3 characters of a command need be typed\n",
	"commands: ? or help \tdisplay commands \n",
	"          ",ccur,"\t",
	"display all current file transfer values \n",
	"          q \t\tleave the ftp program\n",
	"          ",cnote,
	" ok\tgive mail notification on successfull transfer\n",
	"NOTE: values remain set after 'get' or 'send'\n",
	"  values may be cleared by typing the command with no parameter\n",
	"\naccess control: \n   ",
	crhost," <address>\tset address of remote machine\n   ",
	cusr," <login-name>\tset remote login or user name\n   ",
	cpas," <password>\tset remote login or user password\n   ",
	cfilp," <password>\tset remote file password \n   ",
	cacc," <account>\tset remote account\n   ",
	cacp," <password>\tset remote account password\n",
	"\nfile details:\n   ",
	clfiln," <file-name>\tset file pathname  on this system\n   ",
	crfiln," <file-name>\tset file name on remote system\n" ,
	"NOTE: remote and local filenames will default to each ",
	"other if set\n   ",
	0
	};

char    *typehelp[] = {
	ctype," <file-type>\t\tset file type - <file-type> values are:\n",
	"\t",ptia5,"\t\tthe file is an ascii or ia5 file (this is the ",
		"DEFAULT)\n",
	"\t",ptbin,"\t\tthe file is an 8 bit binary file\n" ,
	0
	};

char    *line1[] = {
	"\ntransfer requests:\n  ",
	cwdel,
	" ok\tlocal file will be deleted after a successful transfer\n" ,
	0};

char    *gmodehelp[] = {
	cgmode,
	" <mode>\tdetermine how remote file is read - <mode> values are:\n",
	"\t",pgm1,"\t\tremote file is not deleted  ",
	"(this is the DEFAULT)\n",
	"\t",pgm2,"\t\tremote file is deleted after having been sent\n",
	"\t",pgm3,"\tremote file is deleted as it is being sent\n" ,
	0
	};

char    *smodehelp[] = {
	"   ",csmode," <mode>\tdetermine how remote file is writen",
	"- <mode> values are:\n",
	"\t",psm1,"\t\tsend file only if remote file does not already ",
		"exist\n\t\t(this is the DEFAULT value)\n",
	"\t",psm2,"\t\tsend the file only if the remote file already",
		" exists\n",
	"\t",psm3,"\t\teither make a new file or replace an existing one\n",
	"\t",psm4,"\t\tappend the file to an existing remote file\n",
	"\t",psm5,"\t\teither append the file or make new file on remote",
		" host\n" ,
	0
	};

char    *sendhelp[] = {
	"   ",csend,
	"\t\tsend local-file to remote-file using current values\n",
	"\tthe command will prompt for neccessry parameters not yet set\n   ",
	cget,
	"\t\tget remote-file and place in local-file using current values\n",
	"\tthe command will prompt for neccesary parameters not yet set\n",
	0       /* end of message mark */
	};

char    *notehelp[] = {
	"\t",pnot1,"\tget mail notification if transfer successfull or not\n",
	"\t",pnot2,"\tget mail notification only if transfer fails\n",
	"\t<cr>  turn off successfull notification (same as ",pnot2,")\n" ,
	0};

char    *wdelhelp[] = {
	"\t",pnot1,"\tdelete local file after a successful transfer\n",
	"\t",pnot2,"\tdo not delete local file - DEFAULT value\n",
	"\t<cr>  cancell local file deletion (same as ",pnot2,")\n" ,
	0};

char    **helpall[] = {
	help0,
	typehelp,
	line1,
	gmodehelp,
	smodehelp,
	sendhelp,
	0};

/*must be in same order as in define table below*/
char    **helparms[] = {
	typehelp,
	smodehelp,
	gmodehelp,
	notehelp,
	wdelhelp,
	0};

/*
 * table which assigns a value to a command - the first 11 also correspond
 * with the postition of the data in the queue table
 */
#define HOST  0                 /* remote host */
#define USER  1                 /* remote user (login) name */
#define PASS  2                 /* remote user password */
#define RFILE 3                 /* remote file name */
#define FPAS  4                 /* remote file password */
#define LFILE 5                 /* local file name */
#define ACC   7                 /* remote account */
#define APAS  8                 /* remote account password */
#define DEV   9                 /* device type - for send */
#define OMES 10                 /* message for remote operator*/

/*
 * TYPE,MODES,MODEG,NOTFY & WDEL
 * must be sequential in that order; else can have any values
 */
#define TYPE  11                /* file type */
#define MODES 12                /* mode of send transfer */
#define MODEG 13                /* mode of get transfer */
#define NOTFY 14                /* notify command */
#define WDEL  15                /* delete local file (write-delete) */

#define HELP  16                 /* help command or ? */
#define VALS  17                 /* current values command */
#define SEND  18                /* send command */
#define GET   19                /* get command */
#define QUIT  20                /* quit the ftp program */
#define NOCOM 21                /* blank line input by user */
#define SH    22                /* execute a shell command */

#define MP    70                /* maximum length of command parameter*/

char dummy[1];  /* dummy - to get correct index for commands from coms */

/*
 * table of commands  - the order MUST be so that coms[i],
 *  where i is a defined number above,gives the command defined there
 */
char    *coms[] = {
	crhost,cusr,cpas,crfiln,cfilp,
	clfiln,dummy,cacc,cacp,cdev,
	dummy,ctype,csmode,cgmode,
	cnote,cwdel,
	chelp,ccur,csend,cget,
	0};

char    rhost[MP],user[MP],pass[MP],
	rfil[MP],fpas[MP],lfil[MP],
	acc[MP],apas[MP],dev[MP],
	omes[MP],ftype[7],tsmode[10],
	tgmode[10],
	notval[7],wdelval[7];

char    *values[] = {                   /* table of strings given by user */
	rhost,user,pass,rfil,fpas,
	lfil,dummy,acc,apas,dev,omes,
	ftype,tsmode,tgmode,notval,wdelval,
	0 };

/* table for checking if a users value is one of a fixed set*/
/* if <cr> is a possible response, first item in table should be null strng*/

			/* possible file types */
char    *types[] = {
	ptia5,
	ptbin,
	0};

			/* possible send modes*/
char    *smodes[] = {
	psm1,psm2,
	psm3,psm4,
	psm5,0};

			/*possible make modes*/
char    *gmodes[] = {
	pgm1,pgm2,
	pgm3,
	0};

			/*possible notification values*/
char    *notvals[] = {
	"",pnot1,
	pnot2,
	0};

/*all possible parameters - must be in same order as defines table above */
char    **parms[] = {
	types,smodes,
	gmodes,notvals,
	notvals
	};

struct tab tab;                 /* queue file entry table*/
#define ptrmax &tab.text[TEXTL] /* postion of end of queue entry table*/

/*
 * bits set           mean
 * USER(bit 1)   remote user name has been set by user
 */
#define UNSET 2
/*   RFILE (bit 3)  remote file name has been set by user */
#define RFSET 010
/* LFILE (bit 5)  local file name    "       */
#define LFSET 040

int     flag;
int     u_id,g_id;      /* users real id and group id*/

char    ibuf[256];      /* standard input buffer and pointer*/
char    *iptr;
/* What ?? char    *cp(); */
char    *rindex(), *index(), *tgetent();
char    issecure;

struct  host_entry      *Chp;

main(argc,argv)
char **argv;
{
	int     i;

	ftpinit();   /* initialise program */
	printf("%s type 'help' or a command \n",version);
	for(;;){                        /* until ^c or q given */
		printf("*");            /* give  prompt */
		fflush(stdout);
		switch(i=command()) {   /* get user command */
		case HOST:
		case PASS:
		case DEV:
		case ACC:
		case FPAS:
		case APAS:
			/*
			 * save parameter from command -
			 * don't change flag settings
			 */
			savedata(i,0);
			continue;
		case USER:
		case RFILE:
		case LFILE:
			/*
			 * save parameter - set/clear
			 * flag bit appropriately
			 */
			savedata(i,1);
			continue;
		case TYPE:
		case MODES:
		case MODEG:
		case NOTFY:
		case WDEL:
				/*set and check a fixed value parameter */
			savep(i, i - TYPE);
			continue;
		case SEND:
		case GET:
			transfer(i); /* process and queue a transfer request*/
			continue;
		case HELP:
			help(helpall);        /* give complete help */
			continue;
		case VALS:
			value();        /* display current access values */
			continue;
		case QUIT:
			ftpexit();      /* exit from the program - no return*/
		case NOCOM:
			continue;        /* null line received-prompt again*/
		case SH:
			shell();        /* execute a shell command */
			continue;
		default:
			printf("illegal command - type '?' for help\n");
		}
	}
}

/*append request to queue and start daemon if neccessary */

ab(dir)
{
	register i;

	/* build queue entry table in tab */

	/* work out access setting and put in table */
	if(dir == SEND){ /* first 5 send modes are in access code order*/
		for(i=0;smodes[i] != NULL; i++){
			if(equals(smodes[i],values[MODES])){
				access_m = i+1;
				break;
			}
		}
	}
	else{  /* direction must be GET */
		/* no easy way! */
		if (equals(values[MODEG],"keep"))
			access_m = 0x8002 ; /* hex 8002 */
		else if(equals(values[MODEG],"delete"))
			access_m = 0x8001; /* hex 8001 */
		else  /* must be remove during */
			access_m = 0x8004; /* hex 8004*/
	}


	/* file type */
	if (values[TYPE][0] == 'b')
		t_flags |= BINARY_TRANSFER;  /* binary */
	else
		t_flags &= ~BINARY_TRANSFER; /* ascii */

	/* notification */
	if (values[NOTFY][0] == 'o')
		t_flags |=  NOTIFY_SUCCESS;  /* notify success */
	else
		t_flags &= ~NOTIFY_SUCCESS; /* only notify failure */

	/* delete local file */
	if (values[WDEL][0] == 'o')
		t_flags |= WRITE_DELETE; /* delete local file*/
	else
		t_flags &= ~WRITE_DELETE; /* don't delete local file*/
}

catchsig()
{       /* chatch signals*/
	printf("system error  - please seek help\n");
	ftpexit();
}

	/* get input from user and parse. return value of command */

command()
{
	int     i;

	getline();              /* read in whole line from user */

	if(space() < 0)         /* get passed leading spaces */
		return(NOCOM);  /* no command given */

	/* check first three characters to identify command */
	for (i=0; coms[i] != NULL;i++)
		if(equals(iptr,coms[i]))
			return(i);

	/* single character commands */

	if(*iptr == '?')
		return(HELP);
	if(*iptr == 'q')
		return(QUIT);
	if(*iptr == '!')
		return(SH);
	return(-1);
}

		/* check if first 3 chars of str1 = str2 */
equals(str1,str2)
char *str1,*str2;
{
	if( *str1++ == *str2++ && *str1++ == *str2++ && *str1 == *str2)
		return(1);
	return(0);
}

		/* exit from the ftp program */
ftpexit()
{
	exit(0);
}

		/* initialise program */
ftpinit()
{
	register i;
	char    *strcpy();

	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise. Consult an expert\n");
		exit(1);
	}
	(void) umask(0);        /* allow created file to be rw by all */
				/* catch signals */
	for (i=1;i<14;i++)      /* give message and exit */
		(void) signal(i,catchsig);
	(void) signal(SIGINT,ftpexit);  /* cntrl+c - just exit */

	flag = 0;

	/* set default values */
	u_id = getuid();        /* get users real id */
	g_id = getgid();        /* get users real group-id */

	if( (cur_user = getpwuid(u_id)) == NULL){
		printf("Who are you ????\n");
		exit(1);
	}
	if(getwdir()){  /* get working directory */
		printf("Where are we ??\n");
		exit(1);
	}
	athome = (strcmp(cwd, cur_user->pw_dir) == 0);

	cp("ascii",values[TYPE]);
	cp("make",values[MODES]);
	cp("keep",values[MODEG]);
	cp("not-ok",values[NOTFY]);
	cp("not-ok",values[WDEL]);

	(void) strcpy(values[USER], cur_user->pw_name);
}

		/* read in whole line from user */
getline()
{
	register c;

	for(iptr = ibuf ; ( c = getchar()) != EOF && c != '\n' ; )
		*iptr++ = c;
	*iptr = '\0';
	iptr = ibuf;
	if(c == EOF)
		exit(0);
}

/* process value from get/send request for a value */
/* if value successfully recieved, the 'setting' bits of flag are set  */

getvalue(val,setting)
{
	register int i = 0;

	while(*iptr != ' ' && *iptr != '\0'){
		values[val][i++] = *iptr++;
		if(i > MP - 2){
			printf("value given is too long (%d chars max)\n",MP);
			values[val][0] = '\0';  /* reset value */
			return;
		}
	}
	values[val][i] = '\0';                  /* terminate string */
	flag |= setting;                        /* user has set a value  */
}

		/* dislplay help message  */
help(table)
char **table[];
{
	char **message;
	int i;

	for (i=0; table[i] != NULL ;i++)
		for(message = table[i]; *message ; message++)
			printf("%s",*message);
}

/* get value associated with command comm */
/* if setflag set then set or clear flag appropriate flag bit */

savedata(comm,setflag)
{
	int i;

	while(*iptr++ != ' '&& *iptr != '\0');  /* get passed the command */
	if(space()< 0){                         /* no parameter given */
		printf("%s cleared\n",coms[comm]);
		values[comm][0] = '\0';         /* clear value */
		if(setflag)
			flag &= ~(1<<comm);     /* clear flag bit */
		return;
	}
	if(*iptr == '?'){
		help (helpall);
		return;
	}

	i = 0;
	while(*iptr){                           /* allow embedded spaces */
		values[comm][i++] = *iptr++;
		if(i > MP - 2){
			printf("parameter too long - try again\n");
			return;
		}
	}
					/* strip off trailing spaces */
	while (values[comm][--i] == ' ');
	values[comm][++i] = '\0';

	if(setflag)
		flag |= (1<<comm);              /* set flag bit */

	switch(comm){     /* command dependant acitons  */
	case LFILE:
		if(!(flag&RFSET))
			setfile(RFILE); /*rem file default is local*/
		break;
	case RFILE:
		if(!(flag&LFSET))
			setfile(LFILE); /* & vica versa */
		break;
	}
}

/* save and check a parameter that must have a fixed value*/
/* comm is command given, ind is index relative to 'type' */

savep(comm,ind)
{
	char *q,**message;
	int i;

	while(*iptr++ != ' ' && *iptr != '\0'); /* get passed command */

	if(space()<0){                          /* no value given */
		if(*parms[ind][0] != '\0')      /*  one required*/
			printf("please give parameter or '?' with command\n");
		else {                          /* clear command */
			printf("%s cleared\n",coms[comm]);
			values[comm][0] = '\0';
		}
		return;
	}
	if(*iptr == '?'){                       /* give help */
		for(message = helparms[ind]; *message ; message++)
			printf("%s",*message);
		return;
	}

	for (i=0; parms[ind][i] != NULL;i++){
		q = parms[ind][i];
		if(equals(iptr,q)){     /* compare 1st 3 chars */
			cp(q,values[comm]);/*save full value of parameter*/
			return;
		}
	}
	printf("incorrect value - type '%s ?' for help\n",coms[comm]);
}

/*
 * set remote file name to last part of local file name as
 * default  or local file name to be remote name
 * the file name to be set is 'place' = LFILE or RFILE
 */

setfile(place)
{
	char *p;

	if(place == RFILE){
		if(p = rindex(values[LFILE], '/'))
			p++;
		else
			p = values[LFILE];
		cp(p, values[RFILE]);   /* copy to remote file  */
	}
	else /*can't do fancy things with remote name as structure not known*/
		cp(values[RFILE],values[LFILE]);
}

shell()
{       /* execute a shell command */
	printf(" this facility is not implemented yet\n");
}

/*
 * get past spaces on input line
 * to look for next word
 * return -1 if \n encountered
 */

space()
{
	if(*iptr == '\0')
		return(-1);
	while(*iptr == ' '){
		iptr++;      /* don't advance pointer if 1st char not sp*/
		if(*iptr == '\0')
			return(-1);   /* end of line */
	}
	return(0);
}

/* queue file transfer request */
/* dir gives the direction (send or get) */

transfer(dir)
{
	char i,*string;
	int fil;
	int     j;

	/* check if remote host is set - else prompt */
	while (*values[HOST] == '\0' ){
		printf("%s (<host-name>,c or ?):",crhost);
		getline();
		if(space() <0 )
			continue;       /* keep trying */
		if(*iptr == '?'){
			printf("<host-name>\tname or address of remote machine\n");
			printf("c\treturn to command level\n");
			printf("?\tgive help\n");
			continue;          /* give help */
		}
		if(*iptr == 'c' && (*(iptr+1) == '\0' || *(iptr+1) == ' '))
			return;   /* return to command level */
		getvalue(HOST,0);
	}
	/* check that host name given is acceptable to  host address look up
	routines, and if all ok get transport service initial */

	if(checkhost()){
		values[HOST][0] = 0;
		return;
	}

	/* check if remote user to be set */
	if((flag & UNSET) == 0) /* not set by user */
	do{
getname:
		printf("%s [DEFAULT: %s](<login-name>,<cr>,c or ?):",
			cusr,values[USER]);
		getline();
		if (space() < 0)
			continue; /* empty line - use default (if present)*/
		if(*iptr == 'c' && (*(iptr+1)=='\0' || *(iptr+1)==' '))
			return;   /* return to command level */
		if (*iptr == '?'){
			printf("<login-name>\tset login or user name to <login-name>\n");
			printf("<cr>\tset login name to %s\n",values[USER]);
			printf("c\treturn to command mode\n");
			printf("?\tgive help\n", values[USER]);
			goto getname; /* try again */
		}
		getvalue(USER,UNSET);

	}while(*values[USER] == '\0');

	/* now check the password. always */
	while(*values[PASS] == '\0'){
		printf("%s [DEFAULT: ](<password>,<cr>,c or ?):", cpas);
		noecho();
		getline();
		echo();
		if (space() < 0)
			break; /* empty line - use default (if present)*/
		if(*iptr == 'c' && (*(iptr+1)=='\0' || *(iptr+1)==' '))
			return;   /* return to command level */
		if (*iptr == '?'){
		      printf("<password>\tset user password to <password>\n");
			if(*values[PASS] != '\0')
			    printf("<cr>\tset password to %s\n",values[PASS]);
			printf("c\treturn to command mode\n?\tgive help\n");
			continue;               /* try again */
		}
		getvalue(PASS,0);
		if(*values[PASS] == '\0')
			break;
	}

	/* check if file names are set -do local file then remote file */
	fil = LFILE;
	string ="local";
	for(i=0;i<2;i++){
		while((flag & (1<<fil))==0){        /* name not set by user*/
			if(values[fil][0])
				printf("%s file-name [DEFAULT:%s] (<file-pathname>,<cr>,c, or ?):",
					string,values[fil]);
			else
				printf("%s file-name (<file-pathname>,c, or ?):",string);
			getline();
			if (space() < 0){    /*  <cr> typed  */
				if(values[fil][0])
					break;
				continue;  /* no file name set - try again*/
			}
			if (*iptr == 'c'
				&&(*(iptr+1)=='\0' || *(iptr+1)==' '))
					return; /* back to command mode */
			if(*iptr == '?') {
				if(values[fil][0])
					printf("<cr>\t%s file name will be  %s\n",
						string,values[fil]);
				printf("<file-pathname>\t%s file is <file-pathname>\n",string);
				printf("c\t\treturn to command mode\n" );
				printf("?\t\tgive help\n");
				continue ;
			}
			getvalue(fil,(1<<fil)); /* get local file name */

		}
		fil = RFILE; /* swap to do remote file */
		string = "remote";
		if ((flag & RFSET) == 0) /* if remote file not set, set it*/
			setfile(RFILE);
	}

	/* check out access rights on local file */

#ifdef  UCL
	*realf = 0;
	if((u_id >= EXTERNUSER || *values[LFILE] == '<') && checkperms(dir)){
		printf("No permission to access file %s,", values[LFILE]);
		printf("transfer has not been queued\n");
		return;
	}
#endif
	if(dir == SEND){
		if(*realf)
			j = access(realf, 4);
		else 
			j = access(values[LFILE], 4);
		if(j < 0){
			printf("no permission to read file %s,transfer has not been queued\n",
				values[LFILE]);
			return;
		}
	}
	else    {
		/* trying to write the file */
		if((j = access(values[LFILE], 2)) < 0){
			if(access(values[LFILE], 0) < 0){
				/* cannot write it. Try the directory above */
				char    *p, *rindex();
				if(p = rindex(values[LFILE], '/')){
					if(p == values[LFILE])
						j = access("/", 3);
					else {
						*p = '\0';
						j = access(values[LFILE], 3);
						*p = '/';
					}
				}
				else
					j = access(".", 3);
			}
		}
		if(j < 0){
			printf(
	"no permission to write file %s,transfer has not been queued\n",
							values[LFILE]);
			return;
		}
	}

	/* check with user one last time */
	for(;;){                /* loop till something done! */
		printf("queue request to %s %s ? (<cr>,c,?):",
				dir==SEND?"send":"get", values[LFILE]);
		getline();
		if(space() <0 ) {
			printf("request being queued\n");
			break; /* cr so go ahead  */
		}
		if(*iptr == 'c')
			return; /* return to command mode */
		if( (*iptr == 'y' || *iptr == 'Y') && *(iptr+1) == '\0'){
			printf("request being queued\n");
			break;
		}
		if( (*iptr == 'n' || *iptr == 'N' ) && *(iptr+1) == '\0')
			return;
		if (*iptr == '?') {
			printf("\n<cr>\t go ahead and queue the transfer\n");
			printf("c\treturn to command mode without queueing transfer\n");
			printf("?\tgive help\n");
		}
	}

	/* get to end of appropriate queue */
	/* open queue each time since lftp will delete a queue if
	 * if it finds it is empty and this might happen between
	 * users requests to queue a transfer */

	write_queue(dir);

	*values[LFILE] = '\0'; /* clear out file names */
	*values[RFILE] = '\0';
	flag &= ~(LFSET|RFSET); /* clear flag setting */
}

	/*display current access control and file transfer values */
value()
{
	register int i;

	printf("the following values are set:\n");
	for(i=0; values[i] != NULL ;i++)
		if(values[i][0])
		     printf("\t%s is set to %s\n",coms[i],values[i]);
}

/* What ?? char    * */
cp(from, to)
register char    *from, *to;
{
	while(*to++ = *from++);
}

aa()
{
	DIR     *qdir;
	struct  NETWORK *np;
	struct  QUEUE   *qp;
	char    *q;
	int     i;

	if(issecure && Chp->n_localhost){
		printf("No permission to access host %s\n", values[HOST]);
		return(1);
	}

	if(Chp->n_disabled)
		printf("Warning: Host disabled.\n");

	for(i = 0 ; i < Chp->n_nets ; i++){
		if(Chp->n_addrs[i].net_name == NULL)
			continue;
		else if(Chp->n_addrs[i].ftp_addr != NULL)
			break;
	}
	if(i >= Chp->n_nets /* && !Chp->n_localhost */){
		printf("Host does not support file transfers\n");
		return(1);
	}
	for(np = NETWORKS ; np->Nname ; np++)
		if(strcmp(Chp->n_addrs[i].net_name, np->Nname)==0)
			break;
	if(np->Nname == 0 || np->Nqueue == 0){
		printf("Unknown queue: consult an expert\n");
		return(1);
	}
	for(qp = QUEUES ; qp->Qname ; qp++)
		if(strcmp(qp->Qname, np->Nqueue) == 0)
			break;
	if(qp->Qname == NULL){
		printf("Cannot access queue: consult an expert\n");
		return(1);
	}
	if( (q = qp->Qdir) == NULL)
		q = qp->Qname;
	if(*q != '/')           /* if not a full name add the default path*/
		sprintf(queue, "%s/%s", NRSdqueue, q);
	else
		(void) strcpy(queue, q);

	if( (qdir = opendir(queue))==NULL){
		printf("Cannot access queue, Transfer not queued\n");
		return(1);
	}                               /* now got queue directory open */
	(void) closedir(qdir);          /* so close it */
	return(0);
}


/* generate a local file for placement of queue entry */

get_tmp()
{
	int     pid;
	static  int     extra = 0;
	int     fd;

	pid = getpid();
	do{
		sprintf(rname,"%s/q%d.%d",queue,pid,++extra);
	}while(access(rname, 0) == 0);

	sprintf(ename,"%s/x%d.%d",queue,pid,extra);

	if((fd = creat(ename, (0660)))<0){
		printf("Cannot create queue entry.\n");
		exit(1);
	}
	return(fd);
}


/* routine to read in strings and check options */


struct  acts {
	short   *tabp;
	char    *entryp;
	} act[] = {
	&tab.r_usr_n, user,
	&tab.r_usr_p, pass,
	&tab.r_fil_n, rfil,
	&tab.l_fil_n, lfil,
	&tab.dev_type, devt,
	&tab.dev_tqual, devq,
	&tab.r_fil_p, fpas,
	&tab.l_hname, host_name,
	&tab.l_network, channel,
	&tab.account, acc,
	0,0
	};

write_queue(dir)
{
	register struct acts *ap;
	register char   *tp = tab.text;
	register char   *sp;
	char    *fullpath();
	long    t;

	/* clear tab every time */
	for(sp = (char *)&tab ; sp < (char *)&tab + sizeof(tab)  ;)
		*sp++ =0;
	if(aa())
		return;
	ab(dir);
			/* first set up strings */
	for(ap = act ; ap->tabp ; ap++) /* set up strings */
		if(*ap->entryp){
			*ap->tabp = tp - (char *)&tab;
			if(ap->entryp == lfil)
				sp = fullpath(lfil);
			else
				sp = ap->entryp;
			if(sp == NULL){
				fprintf(stderr, "Directory help\n");
				exit(1);
			}
			while(*tp++ = *sp++);
		}
		else
			*ap->tabp = 0;

	tab.tptr = tp - (char *)&tab;           /* set up others */

	time(&t);
	tab.t_queued = t;
	tab.status = PENDINGSTATE;
	tab.t_access = access_m;
#ifdef UCL /* This probably breaks command (but it compiles) - wja */
	tab.t_flags = t_flags | OLD_PASSWD;
#endif UCL
	if(t_flags & BINARY_TRANSFER && tab.bin_size <= 0 )
		tab.bin_size = 8;
	tab.l_usr_id = u_id;
	tab.l_grp_id = g_id;
	tab.l_docket = 0;

	queuefd = get_tmp();
	if(write(queuefd,(char *)&tab,sizeof(struct tab)) != sizeof(tab)){
		(void) unlink(ename);
		(void) close(queuefd);
		printf("Queue write error. Please seek help\n");
		return;
	}
	(void) close(queuefd);
	t_flags = 0;
	access_m = 0;
	rename(ename,rname);
	tellftspool();

}

#ifndef _42
/* rename a file, This is done as a routine for when 4.2 comes allong */

rename(f1,f2)
char    *f1,*f2;
{
	(void) link(f1,f2);
	(void) unlink(f1);
}
#endif

/*
 * Routine to process flags
 * Most can be put anywhere on the line apart from 'o' and 'x' since
 * these must be given before the part of the name with the host in.
 */

/*
 *      Routine to tell the ftp spooler that a job has been queued.
 *      (this should only occur when there is no daemon running)
 */

tellftspool()
{
	int     fd;
	int     ftp_pid = 0;

	if( (fd = open(NRSdspooler,0))< 0)
		ftp_pid = 0;
	else if(read(fd,(char *)&ftp_pid,sizeof(ftp_pid)) != sizeof(ftp_pid))
		ftp_pid = 0;
	(void) close(fd);
	if(!ftp_pid || kill(ftp_pid,SIGINT)){   /* give it a cntrl-c */
#ifdef  UCL
		if(u_id >= EXTERNUSER)
			return;
#endif
		printf("The transfer has been queued\n");
		printf("But it will not occur immeadiately\n");
	}
}

/*
 * get working directory
 */
#ifdef  _42

getwdir()
{
	if(getwd(cwd) == NULL)
		return(1);
	return(0);
}

#else

getwdir()
{
	int     pipes[2];
	int     pid;
	int     status,i;

	(void) pipe(pipes);     /* create a pipe to pwd */

#ifdef  VFORK
	while((pid = vfork()) < 0)
#else
	while((pid = fork()) < 0)       /* Now get child */
#endif
		sleep(2);

	if(pid == 0 ){                  /* kiddy time */
		(void) close(pipes[0]); /* set up the pipe */
		(void) close(1);
		(void) dup(pipes[1]);
		(void) close(pipes[1]);
					/* now do the exec */
#ifdef  UCL
		execl("/bin/pwd","pwd",0);
		execl("/usr/bin/pwd","pwd",0);
#else
		execlp("pwd","pwd",0);
#endif
		_exit(-1);
	}
					/* the parent */
	(void) close(pipes[1]);
	while(wait(&status)!= pid);     /* wait for it */

	i = read(pipes[0],cwd,256);
	(void) close(pipes[0]);
	if(i <= 0 || *cwd != '/' || status)
		return(1);
	cwd[i-1] = 0;
	return(0);
}

#endif

/* return the full pathname of the string */

char *
fullpath(str)
char *str;
{
	static  char tbuf[256];

	if(*realf){
		strcpy(tbuf, realf);
		*realf = 0;
		return tbuf;
	}
	if(*str == '/')
		(void) strcpy(tbuf,str);
	else
		sprintf(tbuf, "%s/%s", cwd, str);
	return(tbuf);
}


struct  sgttyb  sgl;
int     bits;
int     gotflags;

noecho()
{
	int catchit();

	if(!gotflags){
		signal(SIGINT, catchit);
		signal(SIGQUIT, catchit);
	}
	ioctl(1, TIOCGETP, &sgl);
	bits = sgl.sg_flags & ECHO;
	sgl.sg_flags &= ~ECHO;
	gotflags=1;
	ioctl(1, TIOCSETN, &sgl);
}

echo()
{
	sgl.sg_flags |= bits;
	ioctl(1, TIOCSETN, &sgl);
	if(bits & ECHO)
		printf("\n");
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	gotflags = 0;
}

catchit(sig)
{
	signal(sig, SIG_IGN);
	if(gotflags)
		echo();
	exit(1);
}

checkhost()
{
	char    xbuf[ENOUGH];
	char    **xp;
	struct  NETWORK *np;
	int     i;

	if( (Chp = dbase_get(values[HOST])) != NULL){
		if(Chp->n_context != -1){
			Chp = NULL;
			printf("Unknown host <%s>\n", values[HOST]);
			return(1);
		}
	}
	else {
		for(xp = NRSdomains ; *xp ; xp++){
			(void) strcpy(xbuf, *xp);
			(void) strcat(xbuf, values[HOST]);
			if((Chp = dbase_get(xbuf)) != NULL){
				if(Chp->n_context != -1){
					Chp = NULL;
					printf("Unknown host <%s>\n",
								values[HOST]);
					return(1);
				}
				break;
			}
		}
		if(*xp == NULL){
			printf("Unknown host <%s>\n", values[HOST]);
			return(1);
		}
	}
	(void) strcpy(host_name, Chp->host_alias);
	for(i = 0 ; i < Chp->n_nets ; i++){
		if(Chp->n_addrs[i].net_name == NULL)
			continue;
		if(Chp->n_addrs[i].ftp_addr == NULL)
			continue;
		for(np = NETWORKS ; np->Nname ; np++)
			if(strcmp(np->Nname, Chp->n_addrs[i].net_name) ==0){
			   (void) strcpy(channel, Chp->n_addrs[i].net_name);
				return(0);
			}
	}
	printf("host <%s> does not support NIFTP\n", values[HOST]);
	return(1);
}

#ifdef  UCL
/*
 * check to see if external users are allowed to transfer the file
 * based on the code in the ftp daemon
 */

checkperms(dir)
{
	char    *file = values[LFILE];
	char    tbuf[256];
	FILE    *fp;
	char    *index();
	char    *p;
	int     c;
	int     i;
	extern  char    *SECUREDIRS;

	if(*file == '.' && *(file + 1) == '.')
		return(1);
	for(p = file ; p = index(p, '/') ; p++){
		while(*(p+1) == '/')
			p++;
		if(strncmp(p, "/..", 3) == 0)
			return(1);
	}

	if(*file != '/' && *file != '<')
		return(!athome);
	if(dir != SEND)
		return(1);
	if((fp = fopen(SECUREDIRS, "r")) == NULL)
		return(1);
	for(;;){
		for(p = tbuf ; (c = getc(fp)) != EOF && c != '\n' ; *p++ = c);
		if(c == EOF)
			break;
		*p = 0;
		i = p - tbuf;
		if( (p = index(tbuf, ':')) != NULL){
			*p++ = 0;
			if(*file != '<')
				continue;
			i = p - tbuf - 1;
			if(strncmp(file, tbuf, i) == 0){
				if(file[i-1] != '>')
					continue;
				sprintf(realf, "%s/%s", p, file+i);
				fclose(fp);
				return(0);
			}
			continue;
		}
		if(strncmp(tbuf, file, i))
			continue;
		if(file[i] != '/')
			continue;
		fclose(fp);
		return(0);
	}
	fclose(fp);
	return(1);
}

#endif
