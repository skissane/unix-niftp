/*
 *	x25d.c
 *	Listen for incoming X.25 calls and start up process
 *	Sequent X.25 board (Morningstar Technologies)
 *	Allan Black <allan@uk.ac.strath.cs>
 */

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <utmp.h>
#include <x25/x25.h>
#include <syslog.h>
#include <ctype.h>
#include "nrs.h"

#define MAXSERV		20
#define MAXARG		10
#define MAXENV		100
#define MAXEBYTES	2048
#define MAXCHILD	100
#define BUFSIZE		1024
#define MAXSTR		4096

#define F_PROTID	0
#define F_YBTS		1
#define F_ADAPTOR	2
#define F_PROC		3
#define F_ARGV		4

#define MINFIELDS	4	/* Protid, YBTS, Adaptor and process */
#define MAXFIELDS	MINFIELDS+MAXARG	/* plus MAXARG args */

#define MAXHOST		18	/* Maximum possible name */
#define MAXSHORT	(sizeof nullutmp.ut_host)

#define GFI		0x10

#define YBTSFD		4

#define TS		0x7fffffff
#define TSMASK		0xffffffff
#define X29		0x01000000
#define X29MASK		0xff000000

#define VAR_DTE		0
#define VAR_MYDTE	1
#define VAR_YBTS	2
#define VAR_CYBTS	3
#define VAR_HOST	4
#define VAR_SHORT	5
#define VAR_TTY		6

#define SIZE(a)		((sizeof a)/(sizeof a[0]))

char	x29str[] =	"X.29";

struct protid {
	long	pr_id;
	long	pr_mask;
};

struct service {
	struct protid	*sv_protid;
	char		*sv_ybts;
	int		sv_x29;
	char		*sv_proc;
	char		*sv_argv[MAXARG+1];
};

struct child {
	int	ch_pid;
	char	ch_tty[14];
};

struct vlookup {
	char	*vl_name;
	int	vl_var;
};

struct pidlist {
	char		*pl_name;
	struct protid	*pl_protid;
};

struct protid	x29 = {
	X29,	X29MASK
};

struct protid	ts = {
	TS,	TSMASK
};

struct service servtab[MAXSERV] = {	/* initialised to default table */
	&x29,	"-",		1,	"/etc/getty", {"getty", "$tty"},
	&ts,	"ftp",		0,	"/usr/lib/niftp/qseq", {"janet"},
	&ts,	"ftp.mail",	0,	"/usr/lib/niftp/qseq", {"janet"},
	&ts,	"ftp.news",	0,	"/usr/lib/niftp/qseq", {"janet"},
};

struct pidlist	protids[] = {
	x29str,		&x29,
	"YB",		&ts,
};

int	maxserv;
char	cbuf[BUFSIZE];

struct child	children[MAXCHILD];
struct child	*chend;

char	conffile[] =	"/etc/x25d.conf";
char	tty[] =		"/dev/tty";
char	ttys[] =	"/etc/ttys";
char	utmp[] =	"/etc/utmp";

struct utmp	nullutmp;

char	buf[2051];
int	pktsize;

long	protid;
int	adaptor =	X25_ADAPTER;

char	*fields[MAXFIELDS];
int	maxfields;
char	strbuf[MAXSTR];
int	strbp;

int	x25;
char	svcpath[14];

int	cudflen;
char	cudf[129];
char	mydte[15];
char	dte[15];
char	called[64];
char	calling[64];
char	hostname[MAXHOST+1];
char	shortname[MAXSHORT+1];
char	ttyname[9];

struct vlookup	varnames[] = {
	"host",			VAR_HOST,
	"short",		VAR_SHORT,
	"dte",			VAR_DTE,
	"ybts",			VAR_YBTS,
	"calling_ybts",		VAR_CYBTS,
	"called_dte",		VAR_MYDTE,
	"calling_dte",		VAR_DTE,
	"calling",		VAR_CYBTS,
	"called",		VAR_YBTS,
	"called_ybts",		VAR_YBTS,
	"calling_host",		VAR_HOST,
	"calling_name",		VAR_HOST,
	"calling_shortname",	VAR_SHORT,
	"tty",			VAR_TTY,
	0,
};

char	*var[] = {
		dte,
		mydte,
		called,
		calling,
		hostname,
		shortname,
		ttyname
};

char	*newenv[MAXENV+1];
char	**envp;
char	envbuf[MAXEBYTES];
char	*envbufp = envbuf;
int	envok = 1;

char	*proc;

extern int	errno;
extern int	x25errno;
extern char	**environ;

int		config();
struct service	*getservice();
char		*newstr();
char		*getvar();
char		*lookup();
int		incoming();
int		grimreaper();
char		*index();
char		*rindex();

main(argc, argv)
	int argc;
	char **argv;
{
	int fd;
	if(fork()) exit(0);
	if(proc = rindex(argv[0], '/'))
		proc++;
	else
		proc = argv[0];
	openlog(proc, LOG_CONS, 0);
	for(fd = getdtablesize(); --fd >= 0; close(fd));
	fd = open("/", 0);
	if(fd != 0) {
		close(0);
		dup2(fd, 0);
		close(fd);
		fd = 0;
	}
	dup2(0, 1);
	dup2(0, 2);
	fd = open(tty, 0);
	ioctl(fd, TIOCNOTTY, 0);
	close(fd);
	if((x25 = open("/dev/ttyx0", 2)) < 0) {
		perror("/dev/ttyx0");
		exit(errno);
	}
	signal(SIGX25, incoming);
	signal(SIGCHLD, grimreaper);
	signal(SIGHUP, config);
	config();
	for(;;) pause();
}

config()
{
	struct service *serv;
	FILE *cf;
	if((cf = fopen(conffile, "r")) == NULL) {
		syslog(LOG_CRIT, "%s: %m", conffile);
		return; /* Use default table ? */
	}
	maxserv = 0;
	strbp = 0;
	while(getline(cf, cbuf)) {
		if(maxserv >= MAXSERV) {
			syslog(LOG_NOTICE, "too many services in %s", conffile);
			break;
		}
		if(newserv(&servtab[maxserv], cbuf)) maxserv++;
	}
	fclose(cf);
}

newserv(s, buf)
	register struct service *s;
	char *buf;
{
	register int i;
	if(!split(buf)) return 0;
	for(i = 0; i < SIZE(protids); i++)
		if(strcmp(fields[F_PROTID], protids[i].pl_name) == 0) {
			s->sv_protid = protids[i].pl_protid;
			break;
		}
	if(i >= SIZE(protids)) return 0;
	s->sv_ybts = fields[F_YBTS];
	s->sv_x29 = strcmp(fields[F_ADAPTOR], x29str) == 0;
	s->sv_proc = fields[F_PROC];
	for(i = F_ARGV; i < maxfields; i++) s->sv_argv[i-F_ARGV] = fields[i];
	s->sv_argv[maxfields-F_ARGV] = 0;
	return 1;
}

split(buf)
	register char *buf;
{
	register char *x;
	int last;
	maxfields = 0;
	last = 0;
	while(maxfields < MAXFIELDS) {
		for(x = buf; *x && !isspace(*x); x++);
		if(*x)
			*x++ = 0;
		else
			last++;
		fields[maxfields++] = newstr(buf);
		if(last) {
			if(maxfields < MINFIELDS) {
				syslog(LOG_NOTICE, "not enough fields in %s",
					conffile);
				return 0;
			}
			return 1;
		}
		for(buf = x; isspace(*buf); buf++);
	}
	syslog(LOG_NOTICE, "too many arguments for %s in %s",
		fields[F_PROC], conffile);
	return 0;
}

char *newstr(str)
	char *str;
{
	int len;
	char *x;
	len = strlen(str)+1;
	if(len > MAXSTR-strbp) syslog(LOG_NOTICE, "strbuf too small");
	x = strbuf+strbp;
	strbp += len;
	bcopy(str, x, len);
	return x;
}

getline(f, buf)
	FILE*f;
	char *buf;
{
	register char *x;
empty:
	if(fgets(buf, BUFSIZE, f) == NULL) return 0;
	if((x = index(buf, '#')) == NULL && (x = index(buf, '\n')) == NULL)
		syslog(LOG_CRIT, "%s file corrupt", conffile);
	for(;;) {
		*x-- = 0;
		if(x < buf) break;
		if(!isspace(*x)) break;
	}
	if(buf[0] == 0) goto empty;
	return 1;
}

grimreaper()
{
	register struct child *cp;
	int pid;
	union wait status;
	if(pid = wait3(&status, WNOHANG, 0))
		for(cp = children; cp < &children[MAXCHILD]; cp++)
			if(cp->ch_pid == pid) {
				cp->ch_pid = 0;
				logout(cp->ch_tty);
				return;
			}
}

logout(tty)
	char *tty;
{
	int fd, slot;
	char *ttyid;
	chown(tty, 0, 0);
	chmod(tty, 0666);
	if(ttyid = rindex(tty, '/'))
		ttyid++;
	else
		ttyid = tty;
	if(slot = gettslot(ttyid)) {
		if((fd = open(utmp, 2)) < 0) return;
		strncpy(nullutmp.ut_line, ttyid, sizeof nullutmp.ut_line);
		lseek(fd, (long)(slot*sizeof(struct utmp)), 0);
		write(fd, &nullutmp, sizeof(struct utmp));
		close(fd);
	}
}

gettslot(tty)
	char *tty;
{
	int slot;
	char *x;
	FILE *f;
	char ttbuf[32];
	if((f = fopen(ttys, "r")) == NULL) return 0;
	slot = 0;
	while(fgets(ttbuf, sizeof ttbuf, f)) {
		slot++;
		if(x = index(ttbuf, '\n')) *x = 0;
		if(strcmp(ttbuf+2, tty) == 0) {
			fclose(f);
			return slot;
		}
	}
	fclose(f);
	return 0;
}

incoming()
{
	int lun, lcn, mode, pid, fd;
	char **p;
	char *host, *x;
	struct service *serv;
	getcr(x25);
	if(ioctl(x25, IOX25LUN, &lun) < 0 || lun < 0) return;
	lun >>= 24;
	lun &= 0xff;
	x25device(svcpath, lun, 0);
	if((pid = fork()) < 0) {
		perror("fork");
		return;
	}
	if(pid) {
		if((protid & X29MASK) == X29) child(pid, svcpath);
		return;
	}
	close(x25);
	fd = open(tty, 0);
	ioctl(fd, TIOCNOTTY, 0);
	close(fd);
	if(buf[2] == 0 || buf[2] == CALL_REQUEST) {
		lcn = ((buf[0] & 0xf) << 8) | (buf[1] & 0xff);
		getybts();
		if(x = rindex(svcpath, '/'))
			x++;
		else
			x = svcpath;
		strcpy(ttyname, x);
		if(called[0] == 0) called[0] = '-';
		serv = getservice(protid, called);
		if(serv == (struct service *)0) exit(1);
		if(serv->sv_x29) {
			adaptor = X29_ADAPTER;
			logit("Incoming X.29 from %s\n", dte);
		} else
			logit("Incoming call from %s/%s to %s\n",
				dte, calling, called);
		if((x25 = open(svcpath, 2)) < 0) exit(1);
		if(x25 != YBTSFD) {
			dup2(x25, YBTSFD);
			close(x25);
			x25 = YBTSFD;
		}
		mode = adaptor << 8;
		if(ioctl(x25, IOX25MODE, &mode) < 0) {
			close(x25);
			exit(1);
		}
		if(ioctl(x25, IOX25OPN, &lcn) < 0) {
			close(x25);
			exit(1);
		}
		read(x25, buf, sizeof buf); /* throw away CR packet */
		xaccept(x25);
		if((host = lookup(dte, calling)) == NULL) host = dte;
		strcpy(hostname, host);
		while(strlen(host) > MAXSHORT)
			if(x = index(host, '.'))
				host = x+1;
			else {
				while(*++host);
				host -= MAXSHORT;
				break;
			}
		strcpy(shortname, host);
		for(p = serv->sv_argv; *p; p++)
			if(**p == '$' && (x = getvar(*p+1))) *p = x;
		for(envp = newenv, p = environ;
			envp < &newenv[MAXENV] && *p;
			envp++, p++)
				*envp = *p;
		putenv("X25DTE", dte);
		putenv("YBTSCALLED", called);
		putenv("YBTSTEXT", calling);
		putenv("CALLING_DTE", dte);
		putenv("CALLED_YBTS", called);
		putenv("CALLING_YBTS", calling);
		putenv("CALLED_DTE", mydte);
		putenv("CALLING_NAME", hostname);
		putenv("CALLING_SHORTNAME", shortname);
		if(serv->sv_x29) {
			ioctl(x25, IOX25DATA, &lcn);
			for(fd = getdtablesize(); --fd >= 0;)
				if(fd != x25) close(fd);
			dup2(x25, 0);
			dup2(0, 1);
			dup2(0, 2);
			close(x25);
		}
		execve(serv->sv_proc, serv->sv_argv, newenv);
		logit("execve of %s failed\n", serv->sv_proc);
		exit(1);
	}
	exit(0);
}

struct service *getservice(protid, ybts)
	long protid;
	char *ybts;
	/* Find the service which matches protid and ybts, if any */
{
	register struct service *s;
	for(s = servtab; s < &servtab[maxserv] && s->sv_protid; s++) {
		if((protid & s->sv_protid->pr_mask) == s->sv_protid->pr_id &&
			strcmp(s->sv_ybts, ybts) == 0)
				return s;
	}
	return 0;
}

char *getvar(vname)
	char *vname;
	/* Return the value for a variable, e.g. "$tty" */
{
	register struct vlookup *v;
	for(v = varnames; v->vl_name; v++)
		if(strcmp(v->vl_name, vname) == 0) return var[v->vl_var];
	return 0;
}

putenv(name, val)
	char *name, *val;
	/* put "<name>=<val>" into the new environment */
{
	char *e;
	e = envbufp;
	eputs(name);
	eputc('=');
	eputs(val);
	eputc(0);
	if(envok && envp < &newenv[MAXENV]) *envp++ = e;
}

eputs(str)
	register char *str;
{
	while(*str) eputc(*str++);
}

eputc(c)
	char c;
{
	if(envbufp >= &envbuf[MAXEBYTES]) {
		envok = 0;
		return;
	}
	*envbufp++ = c;
}

child(pid, tty)
	int pid;
	char *tty;
	/* Put a child in the table so we can zap the utmp entry later */
{
	register struct child *cp;
	for(cp = children; cp < &children[MAXCHILD]; cp++)
		if(cp->ch_pid == 0) {
			cp->ch_pid = pid;
			strcpy(cp->ch_tty, tty);
			return;
		}
	/* Should really complain if no space left */
}

getcr(x25)
	int x25;
{
	register int length;
	register char *p, *bp;
	if((pktsize = read(x25, buf, sizeof buf)) < 0) exit(1);
	length = buf[3] & 0xf;
	p = buf+4;
	bp = mydte;
	while(length > 1) {
		*bp++ = ((*p >> 4) & 0xf) + '0';
		*bp++ = (*p & 0xf) + '0';
		length -= 2;
		p++;
	}
	if(length) {
		*bp++ = ((*p >> 4) & 0xf) + '0';
		p++;
	}
	*bp = 0;
	length = (buf[3] >> 4) & 0xf;
	bp = dte;
	while(length > 1) {
		*bp++ = ((*p >> 4) & 0xf) + '0';
		*bp++ = (*p & 0xf) + '0';
		length -= 2;
		p++;
	}
	if(length) {
		*bp++ = ((*p >> 4) & 0xf) + '0';
		p++;
	}
	*bp = 0;
	length = *p++ & 0x3f;
	p += length;
	cudflen = buf+pktsize-p;
	if(cudflen < 0) cudflen = 0;
	strncpy(cudf, p, buf+pktsize-p);
	length = cudflen;
	if(length > 4) length = 4;
	for(p = cudf; length > 0; length--, p++) {
		protid <<= 8;
		protid |= *p & 0xff;
	}
}

getybts()
	/* Should take account of cudflen! */
{
	register int i, length;
	register char *p;
	if(cudflen <= 4) return;
	p = cudf+4;
	if(!(*p & 0x80)) return;
	length = *p++ & 0x3f;
	strncpy(called, p, length);
	p += length;
	if(!(*p & 0x80)) return;
	length = *p++ & 0x3f;
	strncpy(calling, p, length);
	for(p = called; *p; p++) if(isupper(*p)) *p = tolower(*p);
	for(p = calling; *p; p++) if(isupper(*p)) *p = tolower(*p);
}

xaccept(x25)
	int x25;
{
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = CALL_ACCEPT;
	if(write(x25, buf, 3) < 0)
		logit("write CALL_ACCEPT failed (%d/%d)\n", errno, x25errno);
}

char *lookup(dte, ybts)
	char *dte, *ybts;
{
	register struct host_entry *h;
	register char *p;
	char buf[81];
	if(nrs_init() < 0) return NULL;
	sprintf(buf, "%c.%s%s%s", (*dte == '0') ? 'j' : 'p',
		dte, (*ybts) ? "." : "", ybts);
	for(p = buf+strlen(buf); p > buf+2; *--p = 0)
		if(h = dbase_get(buf)) return h->host_alias;
	return NULL;
}

logit(fmt, args)
	char *fmt;
	int args;
{
	FILE *f;
	if((f = fopen("/tmp/x25d.log", "a")) == NULL) return;
	_doprnt(fmt, &args, f);
	fclose(f);
}

