/* unix-niftp bin/setup.c $Revision: 5.5 $ $Date: 90/08/01 13:32:12 $ */
/*
 * setup - maintain users username/passwd file. very basic.
 *
 * $Log:	setup.c,v $
 * Revision 5.5  90/08/01  13:32:12  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:03:05  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:06:06  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:20:47  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:20:18  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include <stdio.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "qfiles.h"

struct  passwd  *getpwuid();
char    *getpass(),*gets();

char    xbuf[100];      /* more work space */
char    wbuf[256];      /* work space */
char    kbuf[1024];
char    iline[256];
char    pbuf[100];      /* somewhere to put the password */
char    mbuf[100];
int     uid;
int     iflag;

main(argc, argv)
char    **argv;
{
	FILE    *Fp,*Ft;
	struct  passwd  *pw;
	char    *p,*docrypt();
	struct  stat    statbuf;
	int     i, j;
	char	*command = argv[0];
	char	*host;
	int	checklen;
	char	*user = (char *) 0;

	if (argc > 2 && !strcmp(argv[1], "-U"))
	{	argv += 2;
		argc -= 2;
		user = *argv;
	}
	if(argc != 2){
		fprintf(stderr,"Usage: setup [ -U <user>] <hostname>\n");
		exit(1);
	}
	host = argv[1];
	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise. seek advise\n");
		exit(1);
	}

	if(*command == '-')
		iflag++;        /* special flag */
	uid = getuid();

	if((pw = getpwuid(uid)) == NULL){
		fprintf(stderr,"Unknown user\n");
		exit(2);
	}

	if (user)
		strcpy(iline, user);
	else if(!iflag){
		printf("User name on host %s:- ", host);
		fflush(stdout);
		if(gets(iline) == NULL){         /* get input from user */
			fprintf(stderr, "Goodbye\n");
			exit(1);
		}
	}
	else	(void) strcpy(iline, pw->pw_name);
	p = getpass("Password on remote host:- ");

	sprintf(pbuf, "%d.%s", uid,p);  /* create a good thing to encrypt*/
	for(p = &pbuf[strlen(pbuf)] ; p < &pbuf[20] ;)
		*p++ = 0;
	setcrypt();
	p = docrypt(pbuf);

	for (i=0,j=0; iline[i]; i++) switch(iline[i])
	{
	case '\\':
	case ':':	pbuf[j++] = '\\'; pbuf[j++] = iline[i]; break;
	default:	pbuf[j++] = iline[i]; break;
	}
	pbuf[j] = '\0';

	if(*iline == 0)
		sprintf(wbuf, "%s:rp=%s\n", host, p);
	else
		sprintf(wbuf, "%s:us=%s:rp=%s\n", host, pbuf, p);
	checklen = strlen(host) + strlen(pbuf) +5;
	sprintf(xbuf,"%s/%s",pw->pw_dir,USERCONFIG);

	Fp = fopen(xbuf,"r");
	/*
	 * stop Gerry making a directory called .confftp
	 * so scewing up the disk.
	 */
	if(Fp != NULL){
		(void) fstat(fileno(Fp), &statbuf);
		if( (statbuf.st_mode & S_IFMT) != S_IFREG){
			fprintf(stderr,"Directory fault\n");
			exit(2);
		}
	}


	/* stop nasties */
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
	sprintf(mbuf, "/tmp/xxsa.%d",getpid());

	if((Ft = fopen(mbuf, "w"))==NULL){
		fprintf(stderr,"Cannot create temporary file\n");
		exit(4);
	}

	if(*host)
		fprintf(Ft, "%s", wbuf);
	if(Fp != NULL)
		while(fgets(kbuf,1024, Fp) != NULL)
			if (strncmp(kbuf, wbuf, checklen))
				fprintf(Ft,"%s", kbuf);
	/* now put the file back */
	fclose(Ft);
	if(Fp != NULL)
		fclose(Fp);

	if((Ft = fopen(mbuf,"r")) == NULL){
		fprintf(stderr,"Cannot reopen tmp file\n");
		(void) unlink(mbuf);
		exit(5);
	}

	(void) unlink(mbuf);
	(void) unlink(xbuf);            /* make certain we can do it */

	if((Fp = fopen(xbuf,"w"))== NULL){
		fprintf(stderr,"Cannot re-open configuration file\n");
		exit(6);
	}

	(void) chown(xbuf,pw->pw_uid,pw->pw_gid);
	(void) chmod(xbuf,0600);
	/* copy it back */

	while(fgets(kbuf,1024, Ft) != NULL) fprintf(Fp, "%s", kbuf);

	fclose(Fp);     /* flush everything */

	/* (void) chmod(xbuf, 0400);       ** tidy up */
	exit(0);
}
