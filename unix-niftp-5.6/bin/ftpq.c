/* unix-niftp bin/ftpq.c $Revision: 5.5 $ $Date: 90/08/01 13:30:28 $ */
/*
 * Ftp queue examination program.
 *  Original version by Dave Frost/Ruth Moulton 79/81
 * Mortally killed and rewritten by Phil Cockcroft 84.
 * Modified to work with the NRS version Phil C. 85.
 *
 * $Log:	ftpq.c,v $
 * Revision 5.5  90/08/01  13:30:28  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:05:17  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:23:07  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:04:47  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:57:48  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:18:34  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "ftp.h"
#include "files.h"
#include "nrs.h"
#include "../version.h"

struct  tab     tab;

int     longlist;       /* produce the long listing */
int	from;		/* show who it is from (su only)	*/
int     doall;          /* print all the entries in the queue	*/
int     domail;         /* print mail transfers */
int     dodone;         /* print even the done ones */
int     mcheck;         /* perform mail checking only */
#include <ctype.h>
#define MAXUSERS        100
int     itemid[MAXUSERS];
int     nitem;
char    *Lhost;         /* Name of host in line */

char    *file(),*getuser(), *fileh(), *railookup();
char    *rindex(), *index();

short   uid,gid;
char    *queue;

struct aitab {
	char    *inem;
	int     ival;
} acctab[] = {
	"mo",    		ACC_MO,
	"makeonly",    		ACC_MO,
	"make",    		ACC_MO,
	"m",    		ACC_MO,

	"ro",    		ACC_RO,
	"replaceonly",    	ACC_RO,
	"replace",    		ACC_RO,
	"r",    		ACC_RO,

	"rom",  		ACC_ROM,
	"replaceormake",  	ACC_ROM,
	"replacemake",  	ACC_ROM,
	"makeorreplace",  	ACC_ROM,
	"makereplace",  	ACC_ROM,
	"rm",   		ACC_ROM,

	"ao",    		ACC_AO,
	"appendonly",  		ACC_AO,
	"append",  		ACC_AO,
	"a",    		ACC_AO,

	"aom",   		ACC_AOM,
	"appendormake",		ACC_AOM,
	"appendmake",		ACC_AOM,
	"makeorappend",		ACC_AOM,
	"makeappend",		ACC_AOM,
	"am",   		ACC_AOM,

	"tji",			ACC_TJI,
	"takejobinput",		ACC_TJI,

	"tjo",			ACC_TJO,
	"takejoboutput",	ACC_TJO,

	"rar",   		ACC_RAR,
	"readandremove",	ACC_RAR,
	"readremove",		ACC_RAR,
	"removeandread",	ACC_RAR,
	"removeread",		ACC_RAR,
	"rr",   		ACC_RAR,
	"gr",   		ACC_RAR,

	"rdo",    		ACC_RDO,
	"readonly",    		ACC_RDO,
	"read",    		ACC_RDO,
	"g",    		ACC_RDO,

	"dr",	  		ACC_DR,
	"destructiveread",	ACC_DR,
	"readdestructive",	ACC_DR,
	"grd",  		ACC_DR,

	"gji",  		ACC_GJI,
	"givejobinput",		ACC_GJI,

	"gjo",  		ACC_GJO,
	"givejoboutput",	ACC_GJO,

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
	"re-Q", REQSTATE,
	0,      0
};

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
#ifdef	UCL
				if(uid < EXTERNUSER)
#endif
				doall++;
				break;
			case 'd':
			case 'D':
				dodone++; /* display non viable entries too*/
				break;
			case 'f':
			case 'F':
				from++; /* display who sent it if mail & su */
				break;
			case 'l':
			case 'L':
				longlist++;
				break;
			case 'm':
			case 'M':
#ifdef	UCL
				if(uid < EXTERNUSER)
#endif
				domail++;
				break;
			case 's':
			case 'S':
				mcheck++;
				break;
			case 'Q':
			case 'q':
				if(*(p+1)){
				       printf("Bad flag placement (%c)\n",*p);
					break;
				}
				if(argc < 2){
					printf("Queue name required\n");
					break;
				}
				if(queue){
					printf("Queue already specified\n");
					break;
				}
				if((queue = rindex(*++argv, '/')) == NULL)
					queue = *argv;
				else queue++;
				argc--;
				break;
			default:
				printf("Unknown flag (%c)\n",*p);
				break;
			}
		}

#ifdef  UCL
	if(!mcheck)
	printf("%s: File Transfer Queue Status\n", VERSION);
#endif

	printinfo(argc, argv);
	exit(0);
}

char    *Lqueue;        /* local queue name */

printinfo(argc, argv)
char **argv;
{
	DIR     *dirp;
	register struct  QUEUE   *Qp;
	register char    *q;
	char    xbuff[ENOUGH];

	/*
	 * now check hosts if we want them
	 */
	if(argc > 0 && argv && *argv) check_args(argc, argv);

	for(Qp = QUEUES ; Qp->Qname ; Qp++){
		if(queue && strcmp(queue, Qp->Qname))	continue;
		if (Qp->Qmaster)			continue;

		if (!(q = Qp->Qdir)) q = Qp->Qname;
		if(*q != '/'){
			sprintf(xbuff, "%s/%s", NRSdqueue, q);
			q = xbuff;
		}
		Lqueue = Qp->Qname;
		if(chdir(q) < 0 || (dirp = opendir(".")) == NULL){
			printf("Cannot access queue %s\n", Qp->Qname);
			continue;
		}
		if(mcheck){
			mprint(dirp);
			fflush(stdout);
		}
		else
			printentrys(dirp);
		(void) closedir(dirp);
	}
	if(mcheck && !queue)
		mjprint();
}

#define	RM	(30)	/* round to minute */
#define	RH	(30*60)	/* round to hour */

char *timetilnext(next)
long next;
{
#ifdef	ALWAYSREQ
	return	"re-q";
#else	ALWAYSREQ
	static	char buff[5];
	next -= time((long *) 0);

	if (next < 0) return "re-q";

	if (next < 100) sprintf(buff, "%ds", next);
	else if (next+ RM < 100 * 60) sprintf(buff, "%dm", (next +RM) / 60);
	else if (next+ RH < 100 * 60) sprintf(buff, "%dh", (next +RH) / (60*60));
	else return "held";
	return buff;
#endif	ALWAYSREQ
}

/* print out entries in the queue. Do this by searching the directory
 * then sorting the elements
 */

struct  Xinfo {
	char    Xname[14];      /* name of queue entry */
	long    Xtime;          /* last modified time */
	};

#define MAXQ    1500             /* will only process the first 1500 entries */
				/* of the queue */
struct  Xinfo   *Xinf[MAXQ];
int     Xcount;

static  header;         /* if we have printed out the header */

printentrys(dirp)
register DIR     *dirp;
{
	register struct direct *dp;
	register fd;
	int     tstatus;
	register struct Xinfo   *Xp, **XXp;
	struct  stat    statbuf;
	long    xtime;
	int     cmp();
	char    *Ltime(), *Sname();
	char    *malloc();
	char    *strcpy();

	for(Xcount = 0; Xcount < MAXQ ; Xinf[Xcount++] = 0)
		if(Xinf[Xcount])
			free(Xinf[Xcount]);
	Xcount = 0;
	XXp = Xinf;
	/* First read in the directory */
	for(dp = readdir(dirp) ; dp ; dp = readdir(dirp) ){
		if(*dp->d_name != 'q')  /* ignore . and .. */
			continue;
		if(nitem && !checkitem(dp->d_name))		continue;
		if(stat(dp->d_name, &statbuf) < 0)
			continue;
		Xcount++;
		if((Xp = (struct Xinfo *)malloc(sizeof(struct Xinfo)))==NULL){
			printf("Out of core\n");
			exit(3);
		}
		(void) strcpy(Xp->Xname,dp->d_name);
		Xp->Xtime = statbuf.st_ctime;
		if(XXp >= &Xinf[MAXQ-1]){
			qsort(Xinf, MAXQ, sizeof(struct Xinfo *), cmp);
			free(*XXp);
			XXp--;
		}
		*XXp++ = Xp;
	}
	if(Xcount > MAXQ){
		printf(
"%d entries in queue %s - only %d entries shown\n",Xcount, Lqueue, MAXQ);
		Xcount = MAXQ;
	}
	if(Xcount)
		qsort(Xinf,Xcount,sizeof(struct Xinfo *),cmp);
	for(XXp = Xinf ; XXp < &Xinf[Xcount] ; XXp++){
		int id;
		if((Xp = *XXp) == NULL)         /* paranoia */
			continue;
		if((fd = open(Xp->Xname, 0)) < 0 ||
			   read(fd,(char *)&tab,sizeof(tab))!=sizeof(tab)){
			(void) close(fd);
			continue;
		}
		(void) close(fd);
		tstatus = tab.status;   /* get status of entry */

		/* see which entries to do */

		/* Ignore done entries */

		if(Lhost && strcmp(Lhost, tab.l_hname + (char *)&tab))
			continue;
		if(!doall && uid && uid != tab.l_usr_id)
			continue;
		switch(tstatus){
		case DONESTATE:
		case ABORTSTATE:
		case REJECTSTATE:
		case CANCELSTATE:
			if(!dodone)
				continue;
			break;
		default:
			break;
		}

		if(!domail && tab.r_usr_n == 0)
			continue; /* dont print mail unless required */

		if(!header){            /* print out header */
			header++;
			printf("    ID TIME  QUEUED L-FILE         STATE");
			if(longlist)
			      printf(" USER   R-FILE           ADDRESS   MODE");
			printf("\n");
		}
		/*
		 *  Finally , Print entry
		 */
		xtime = tab.t_queued ? tab.t_queued : Xp->Xtime;
		id = atoi(&(Xp->Xname[1]));
		printf("%6d %s %-15.15s",
			id, Ltime(xtime), file(tab.l_fil_n, 15));
		printf(" %-4.4s", (tstatus == REQSTATE) ?
			timetilnext(tab.l_nextattmpt) : 
			railookup(tstatus,statuss));
		if(longlist)
		{
			int	rem = tab.r_fil_n;
			char	*remote;
			int	mail = 0;
			int	userlen = 6;
			int	remotelen=16;
			int	spacelen=1;
			int	hostlen=10;
			char	*hostname = Sname(fileh(tab.l_hname));
			char	*user;

			if (rem)
				remote = file(rem, remotelen);
			else switch (tab.t_flags & T_TYPE)
			{	case	T_FTP:	remote = "<unknown>";	break;
				case	T_MAIL:	mail++;
						remote = "<Mail>";	break;
				case	T_JTMP:	remote = "<JTMP>";	break;
				case	T_NEWS:	remote = "<News>";	break;
				default:	remote = "<Unknown>";	break;
			}

			if (!uid && from && mail && tab.l_from)
			{	userlen += remotelen + spacelen;
				spacelen = remotelen = 0;
			}

			user =
#ifdef	OMITUSER
				(uid && tab.l_user_id != uid) ? "-" :
#endif	OMITUSER
				(!remotelen) ? (char *) &tab + tab.l_from :
				getuser(tab.l_usr_id);
			{	int	want[3];
				int	*got[3];
				int i=0;

				want[i]	 = strlen(hostname);
				got[i++] = &hostlen;
				want[i]  = (remotelen) ? strlen(remote) : 0;
				got[i++] = &remotelen;
				want[i]  = strlen(user);
				got[i++] = &userlen;
				evenout(i, want, got);
				if (longlist > 1) while(--i>= 0)
					if (want[i] > *got[i])
						*got[i] = want[i];
			}
			printf(" %-*.*s%*s%-*.*s %-*.*s %s",
				userlen, userlen, user,
				spacelen, "",
				remotelen, remotelen, remote,
				hostlen, hostlen, hostname,
				railookup(tab.t_access & 0xFFFF ,acctab));
		}
		printf("\n");
	}
}

checkitem(file)
char *file;
{	int i;

	/* HACK -- just do an atoi on it .... */
	for(i = 0 ; i < nitem ; i++) if(atoi(file+1) == itemid[i]) return(1);
	return(0);
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
 */
	strncpy(xbuf, &(p[4]), 12);
	xbuf[12] = 0;
	return(xbuf);
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
	if( (p = getpwuid(uid)) == (struct passwd *)0) {
		olduid = -1;
		return(Unknown);
	}
	olduid = uid;
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

cmp(x,y)
struct Xinfo **x,**y;
{
	if((*x)->Xtime > (*y)->Xtime)
		return(1);
	else if( (*x)->Xtime < (*y)->Xtime)
		return(-1);
	return(0);
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
	{	if((hp = dbase_find(*argv, (char *) 0, 0)) != NULL)
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

struct  hfo     {
	struct  hfo     *h_nxt;
	int     h_mc;
	int     h_fc;
	long    h_ot;
	long    h_nt;
	char    h_n[20];
} *HFO;

#define AVXFILES        5       /* maximum number of 'x' files */
long    m_oldest;

mprint(dirp)
register DIR     *dirp;
{
	int     nfile = 0;
	int     nmail = 0;
	int     ncount = 0;
	int     nxfile = 0;
	int     fd;
	char    *p;
	long    t;
	register struct hfo     *hp;
	register struct direct  *dp;

	while( (dp = readdir(dirp)) != NULL){
		if(*dp->d_name != 'q'){ /* ignore '.' and '..' */
			if(*dp->d_name == 'x')
				nxfile++;
			continue;
		}
		if( (fd = open(dp->d_name, 0)) < 0)
			continue;
		if(read(fd, (char *)&tab, sizeof(tab)) != sizeof(tab)){
			(void) close(fd);
			continue;
		}
		(void) close(fd);
		switch(tab.status){
		case DONESTATE:
		case ABORTSTATE:
		case REJECTSTATE:
		case CANCELSTATE:
			continue;
		}
		p = (char *)&tab + tab.l_hname;
		for(hp = HFO ; hp ; hp = hp->h_nxt)
			if(strcmp(hp->h_n, p) == 0)
				break;
		if(hp == 0){
			hp = (struct hfo *)malloc(sizeof(struct hfo));
			if(hp == NULL){
				fprintf(stderr, "Out of core\n");
				exit(1);
			}
			(void) strcpy(hp->h_n, p);
			hp->h_fc = hp->h_mc = 0;
			hp->h_ot = hp->h_nt = tab.t_queued;
			hp->h_nxt = HFO;
			HFO = hp;
		}
		/*
		 * now set oldest and newest times
		 */
		if(tab.t_queued < hp->h_ot)
			hp->h_ot = tab.t_queued;
		if(tab.t_queued > hp->h_nt)
			hp->h_nt = tab.t_queued;
		ncount++;
		if(tab.r_usr_n == 0){   /* mail traffic */
			nmail++;
			hp->h_mc++;
			/*
			 * find the oldest mail file in the whole system
			 */
			if(!m_oldest || m_oldest > tab.t_queued)
				m_oldest = tab.t_queued;
		}
		else {
			nfile++;
			hp->h_fc++;
			continue;
		}
	}
	/*
	 * print out the useful stuff
	 */
	printf("Queue %s:-\n", Lqueue);
	if(ncount == 0)
		return;
	if(nmail != 0 && nfile != 0)
		printf("%d entries in the queue (%d file + %d others)\n",
							ncount, nfile, nmail);
	else if(nmail != 0)
		printf("%d other entries in the queue\n", nmail);
	else
		printf("%d file entries in the queue\n", nfile);
	if(nxfile > AVXFILES)
		printf("WARNING:- queue contains %d 'x' files\n", nxfile);
	time(&t);
	t -= (long)24*60*60;    /* more than a day old */
	while(hp = HFO){
		char    ibuf[20];

		strcpy(ibuf, Ltime(hp->h_ot));
#ifdef  notdef
		printf(
		   "%-18.18s %2d file + %2d other (oldest %s, newest %s)\n",
			hp->h_n, hp->h_mc, hp->h_fc, ibuf, Ltime(hp->h_nt));
#endif
		printf("%-18.18s %2d file + %2d other",
					hp->h_n, hp->h_fc, hp->h_mc);
		if(hp->h_ot)
		{
			printf(" (oldest %s)", ibuf);
			if(hp->h_ot && hp->h_ot < t)
				printf(" ** BLOCKING **");
		}
		printf("\n");
		HFO = hp->h_nxt;
		free( (char *)hp);
	}
}

mjprint()
{
	register DIR   *dirp;
	register struct direct  *d;
	struct  stat    statbuf;
	int     nfile = 0;

	if(chdir(MAILDIR) < 0 || (dirp = opendir(".")) == NULL){
		fprintf(stderr, "Cannot access %s\n", MAILDIR);
		exit(1);
	}
	while( (d = readdir(dirp)) != NULL){
		if(*d->d_name != 'n')   /* skip '.' and '..' */
			continue;
		if(stat(d->d_name, &statbuf) < 0){
			printf("Cannot stat %s\n", d->d_name);
			continue;
		}
					/* 10 = time for cpf to execute */
		if(m_oldest != 0 && statbuf.st_mtime > m_oldest - 10L)
			continue;       /* this one is probarbly ok */
		if(nfile++ == 0)
			printf("Orphaned mail files:-\n");
		printf("%s\n", d->d_name);
	}
	if(nfile != 0)
		printf("Total number of orphans = %d\n", nfile);
	closedir(dirp);
}


/* We have strings of actual length want[x] which we are about to print
 * in width *got[x] -- can we shuffle things around ?
 */
evenout(n, want, got)
int want[], *got[];
{	int i, j;
	for (i=0; i<n; i++)
	{	int err = want[i] - *(got[i]);
		int spare;

		if (err > 0) for (j=0; j<n; j++)
			if ((spare = ((*(got[j])) - want[j])) > 0)
		{	if (spare > err) spare = err;
			*(got[j]) -= spare;
			*(got[i]) += spare;
			err       -= spare;
		}
	}
}
