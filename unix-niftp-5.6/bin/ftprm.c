/* unix-niftp bin/ftprm.c $Revision: 5.5 $ $Date: 90/08/01 13:30:31 $ */
/*
 * Ftprm - delete entries from the queues.
 * Mortally killed and totally rewritten by Phil Cockcroft 84. from code by
 * Dave Frost and Ruth Moulton.
 * Modded to work with the NRS, Phil Cockcroft March 85.
 *
 * $Log:	ftprm.c,v $
 * Revision 5.5  90/08/01  13:30:31  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:10:09  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:05:09  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:58:13  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:18:42  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "opts.h"
#include "tab.h"
#include "files.h"
#include "nrs.h"

struct  tab     tab;

#include <sys/stat.h>
#include <ctype.h>

char    *index(), *malloc(), *strcpy();
char    *rindex();

int     doall;          /* print all the entries in the queue */
int     domail;         /* print mail transfers */
int     doint;
int     longlist;       /* produce the long listing */
char    *Lhost;         /* Name of host in line */
short   uid,gid;
char    *queue;

#define MAXUSERS        100

char    *users[MAXUSERS]; /* delete a users requests only */
int     usersid[MAXUSERS];
int     nusers = 0;
int     itemid[MAXUSERS];
int     nitem = 0;

main(argc,argv)
char    **argv;
int     argc;
{
	register char   *p;

	if(nrs_init() < 0){
		fprintf(stderr, "Cannot initialise. Consult an expert\n");
		exit(2);
	}
	uid = getuid();
	gid = getgid();

	/* first process any flags */

	for(argv++, argc-- ; argc && **argv == '-'; argv++, argc--)
		for(p = *argv + 1 ; *p ; p++){
			switch(*p){
			case 'a':
			case 'A':
				doall++;
				break;
			case 'm':
			case 'M':
				domail++;
				break;
			case 'Q':
			case 'q':
				if(*(p+1)){
				       fprintf(stderr, "Bad flag placement (%c)\n",*p);
					break;
				}
				if(argc < 2){
					fprintf(stderr, "Queue name required\n");
					break;
				}
				if(queue){
					fprintf(stderr, "Queue already specified\n");
					break;
				}
				if((queue = rindex(*++argv, '/')) == NULL)
					queue = *argv;
				else queue++;
				argc--;
				break;
			case 'u':
			case 'U':
				if(argc < 2){
					fprintf(stderr, "-U flag: Users required\n");
					break;
				}
				if(nusers >= MAXUSERS){
					fprintf(stderr, "Only %d users allowed\n",
								   MAXUSERS);
					break;
				}
				users[nusers++] = *++argv;
				argc--;
				break;
			case 'i':
			case 'I':
				doint++;
				break;
			case 'l':
			case 'L':
				longlist++;
				break;
			default:
				fprintf(stderr, "Unknown flag (%c)\n",*p);
				break;
			}
		}

	dodelete(argc, argv);
}

dodelete(argc, argv)
char **argv;
{
	DIR     *dirp;
	register char   *q;
	register struct QUEUE   *qp;
	char    xbuff[ENOUGH];

	if(argc > 0) check_args(argc, argv);

	for(qp = QUEUES ; qp->Qname ; qp++){
		if(queue && strcmp(qp->Qname, queue))	continue;
		if (qp->Qmaster)			continue;
		if( (q = qp->Qdir) == NULL)	q = qp->Qname;
		if(*q != '/'){
			sprintf(xbuff, "%s/%s", NRSdqueue, q);
			q = xbuff;
		}
		if(chdir(q) < 0 || (dirp = opendir(".")) == NULL){
			fprintf(stderr, "Cannot access queue %s\n", qp->Qname);
			continue;
		}
		deleteentries(dirp);
		(void) closedir(dirp);
	}
}

/* delete the entries in the queue */

deleteentries(dirp)
register DIR     *dirp;
{
	register struct direct *dp;
	register fd;

	/* read in the directory */
	for(dp = readdir(dirp) ; dp != NULL ; dp = readdir(dirp) ){
		if(*dp->d_name != 'q')  /* ignore . and .. */	continue;
		if(nitem && !checkitem(dp->d_name))		continue;

		if((fd = open(dp->d_name, 0)) < 0 ||
			   read(fd,(char *)&tab,sizeof(tab))!=sizeof(tab)){
			(void) close(fd);
			continue;
		}
		(void) close(fd);

	/* see which entries to do */

		if(Lhost && strcmp(Lhost, tab.l_hname + (char *)&tab))
			continue;

		if(uid && tab.l_usr_id != uid)			continue;
		if(nusers && !checkuser(tab.l_usr_id))		continue;
		if(!uid && tab.l_usr_id && !doall)		continue;
		if(!domail && tab.r_usr_n == 0)			continue;
		if(doint && askuser(dp) == 0)			continue;

		(void) unlink(dp->d_name);
	}
}

#include <pwd.h>

checkuser(uid)
int     uid;
{
	struct passwd *getpwnam();
	register struct passwd *p;
	static int olduid = -1;
	register int     i;

	if(olduid == -1 )
		for(olduid = i = 0 ; i < nusers ; i++){
			if(p = getpwnam(users[i]))
				usersid[i] = p->pw_uid;
			else
				usersid[i] = -1;
		}
	for(i = 0 ; i < nusers ; i++)
		if(uid == usersid[i])
			return(1);
	return(0);
}

checkitem(file)
char *file;
{	int i;

	/* HACK -- just do an atoi on it .... */
	for(i = 0 ; i < nitem ; i++) if(atoi(file+1) == itemid[i]) return(1);
	return(0);
}


struct aitab {
	char    *inem;
	int     ival;
} acctab[] = {
	"m",    0x1,            /* Make */
	"r",    0x2,            /* Replace */
	"rm",   0x3,            /* Replace or Make */
	"a",    0x4,            /* Append */
	"am",   0x5,            /* Append or Make */
	"tji",  0x2001,         /* Take Job Input */
	"tjo",  0x4001,         /* Take Job Input */
	"g",    0x8002,         /* Get */
	"gr",   0x8001,         /* Get then Remove */
	"grd",  0x8004,         /* get and remove while copying */
	 0,     0
}, statuss[] = {                /* various statuses */
	"t/o",  TIMEOUTSTATE,
	"can",  CANCELSTATE,
	"rej",  REJECTSTATE,
	"ab",   ABORTSTATE,
	"done", DONESTATE,
	"go",   GOSTATE,
	"try",  TRYINGSTATE,
	"pend", PENDINGSTATE,
	"re-q", REQSTATE,
	0,      0
};

static  header;         /* if we have printed out the header */

askuser(dp)
register struct direct *dp;
{
	int     tstatus;
	char    xbuff[100], *p;
	char    *file(), *fileh(), *getuser(), *railookup();
	char    *Ltime(), *Sname();
	int	id;

	tstatus = tab.status;       /* get status of entry */
	switch(tstatus){
	case DONESTATE:
	case CANCELSTATE:
	case REJECTSTATE:
	case ABORTSTATE:
		return(1);
	}

	if(!header){            /* print out header */
		header++;
		printf("    ID TIME  QUEUED L-FILE     STATE");
		if(longlist)
			printf(" USER     R-FILE     ADDRESS   MODE");
		printf("\n");
	}
	/*
	 *  Finally , Print entry
	 */
	id = atoi(&(dp->d_name[1]));
	printf("%6d %s %-11.11s", id, Ltime(tab.t_queued),
		file(tab.l_fil_n, 11));
	printf(" %-4.4s", railookup(tstatus,statuss));
	if(longlist)
	{
		int	rem = tab.r_fil_n;
		char	*remote;

		if (rem)
			remote = file(rem, 10);
		else switch (tab.t_flags & T_TYPE)
		{	case	T_FTP:	remote = "<unknown>";	break;
			case	T_MAIL:	remote = "<Mail>";	break;
			case	T_JTMP:	remote = "<JTMP>";	break;
			case	T_NEWS:	remote = "<News>";	break;
			default:	remote = "<Unknown>";	break;
		}
		printf(" %-8.8s %-10.10s %-9.9s %-5.5s",
#ifdef	OMITUSER
			(uid && tab.l_user_id != uid) ? "-" :
#endif	OMITUSER
			getuser(tab.l_usr_id),
			remote, Sname(fileh(tab.l_hname)),
			railookup(tab.t_access & 0xFFFF ,acctab));
	}
	printf(":? ");
	fflush(stdout); /* jpo@nott.cs - fix for bsd4.2 stdio */
	if(gets(xbuff) == NULL)
		return(0);
	for(p = xbuff ; *p == ' ' || *p == '\t' ; p++);
	if(*p == 'y' || *p == 'Y')
		return(1);
	return(0);
}

static  char Unknown[]  = "<unknown>";

char *
file(addr, len)
unsigned addr;
{
	char    *q,*p;

	if(!addr || (unsigned)addr >= sizeof(tab))
		return(Unknown);
	q = (char *)&tab + addr;
	if(q < tab.text || q >= &tab.text[TEXTL])
		return(Unknown);

	/* Lets see what will fit ... */
	p = q + strlen(q);
	/* Hmmm not all */
	if (p-q > len)
	{	char *slash  = index(p-len, '/');
		if (!slash || p[-len-1] == '/' || (slash - p + len) > 2)
			return p-len;
		return &(slash[1]);
	}

	return(q );
}

char *
fileh(addr)
unsigned addr;
{
	char    *q,*p;

	if(!addr || (unsigned)addr >= sizeof(tab))
		return(Unknown);
	q = (char *)&tab + addr;
	if(q < tab.text || q >= &tab.text[TEXTL])
		return(Unknown);
	if(p = rindex(q, '/'))
		return(p+1);
	return(q );
}

char    *
getuser(uid)
{
	struct passwd *getpwuid();
	static struct passwd *p;
	static int olduid = -1;

	if(uid == olduid)
		return(p->pw_name);
	olduid = uid;
	p = getpwuid(uid);
	return(p->pw_name);
}


char *
railookup(n,p)
register struct  aitab   *p;
{
	for(;p->inem;p++)
		if(p->ival == n)
			return(p->inem);
	return("");
}


char *
Ltime(xtime)
long    xtime;
{
	char    *ctime();
	static  char    xbuf[15];
	register char   *p = ctime(&xtime);
/*
 * 012345678901234567890123456789
 * Sun Sep 16 01:03:52 1973\n
 */
	xbuf[0] = *(p+11);
	xbuf[1] = *(p+12);
	xbuf[2] = *(p+13);
	xbuf[3] = *(p+14);
	xbuf[4] = *(p+15);
	xbuf[5] = *(p+10);
	xbuf[6] = *(p+4);
	xbuf[7] = *(p+5);
	xbuf[8] = *(p+6);
	xbuf[9] = *(p+7);
	xbuf[10] = *(p+8);
	xbuf[11] = *(p+9);
	xbuf[12] = 0;
	return(xbuf);
}

check_args(argc, argv)
char **argv;
{
	struct host_entry       *hp;

	for (; argc > 0; argc--, argv++) if (isdigit(**argv))
	{	if (nitem < MAXUSERS)
			itemid[nitem++] = atoi(*argv);
		else	fprintf(stderr, "Id %s ignored\n", *argv);
	}
	else
	{	if((hp = dbase_get(*argv, (char *) 0, 0)) != NULL)
		{	if (Lhost)
				fprintf(stderr, "Host %s ignored\n", *argv);
			else	Lhost = hp->host_alias;
			continue;
		}
		fprintf(stderr, "Unknown host %s\n", *argv);
		exit(1);
	}
}

char    *
Sname(name)
char    *name;
{
	register char    **xp, *p, *q;
	for(xp = NRSdomains ; (p = *xp) ; xp++){
		for(q = name ; *p && *q++ == *p++; );
		if(!*p)
			return(q);
	}
	return(name);
}
