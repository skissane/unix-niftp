/* unix-niftp lib/gen/dbm.c $Revision: 5.6 $ $Date: 1991/06/07 17:00:35 $ */
/*
 * $Log: dbm.c,v $
 * Revision 5.6  1991/06/07  17:00:35  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:34:21  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:16:51  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0  87/03/23  03:35:41  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "opts.h"

#ifdef RMTDBM
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#undef NULL
#endif RMTDBM

#include "hash.h"

dbase   hashdbm;

#ifdef RMTDBM
char   *index();
static sendcmd();
static getreply();
static int  skt = -1;
extern	errno;
#endif RMTDBM
extern char *strcpy (), *strncpy ();

dbminit(file)
char *file;
{
#ifdef RMTDBM
	struct sockaddr_in  sin;
	struct hostent *hp;
	struct servent *sp;
	char	*cp;
	char	*mch;
	char	dbnam[512];
	char	*servers = file;
	char	*next = (char *) 0;

	if (skt >= 0) close(skt);

	/* Now loop through the comma separated list of servers */
	while (servers && *servers)
	{	char * next = index(servers, ',');
		int len;
		int res;

		if (next)	len = next++ - servers;
		else		len = strlen(servers);

		/* Copy the current string into the buffer */
		strncpy(dbnam, servers, len);
		dbnam[len] = '\0';
		servers = next;

		/* @ -> use remore dbm */
		if ((mch = index(dbnam, '@')) != NULL)
		{	*mch++ = '\0';
			if ((hp = gethostbyname(mch)) == 0)
			{	fprintf(stderr, "unknown host %s\n", mch);
				continue;
			}
			bzero ((char *) & sin, sizeof (sin));
			if ((sp = getservbyname("rmtdbm", "tcp")) == 0)
			{	fprintf(stderr, "rmtdbm service unknown\n");
				continue;
			}
			bcopy(hp->h_addr, (char *) &sin.sin_addr, hp->h_length);
			sin.sin_family = hp->h_addrtype;
			sin.sin_port = sp->s_port;
			if ((skt = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			{	perror("cannot create socket");
				continue;	
			}
			if (connect(skt, (char *) & sin, sizeof (sin)) < 0)
			{	perror("cannot connect socket");
				close(skt); skt = -1;
				continue;
			}
			sendcmd(skt, 'O', dbnam);
			res = getreply(skt, 'O', 0);
			if (res == -1)
			{	perror("Failed to open remote dbm");
				close(skt); skt = -1;
				continue;
			}
		}
		else
		{	res = hashinit(&hashdbm, dbnam);
			if (res == -1)
			{	perror("Failed to open local dbm");
				continue;
			}
		}
		return res;
	}
	fprintf(stderr, "Failed to find a usable dbm in `%s'\n", file);
	return -1;
#else	/* RMTDBM */
	return hashinit(&hashdbm,file);
#endif	/* RMTDBM */
}

dbmclose()  {
#ifdef RMTDBM
	if (skt >= 0)
	{	int  s;
		sendcmd(skt, 'C');
		s = getreply(skt, 'C', 0);
		close(skt); skt = -1;
		return s;
	}
#endif RMTDBM
	return hashclose(&hashdbm);
}

datum fetch(key)
datum   key;
{
#ifdef RMTDBM
	if (skt >= 0)
	{	static datum  data;
		sendcmd(skt, 'F', &key);
		getreply(skt, 'F', &data);
		return data;
	}
#endif RMTDBM
	return hashfetch(&hashdbm, key);
}

delete(key)
datum key;
{
#ifdef RMTDBM
	if (skt >= 0) return -1;
#endif RMTDBM
	return hashdelete(&hashdbm, key);
}

store(key, dat)
datum   key;
datum   dat;
{
#ifdef RMTDBM
	if (skt >= 0) return -1;
#endif RMTDBM
	return hashstore(&hashdbm, key, dat);
}

datum firstkey()  {
#ifdef RMTDBM
	if (skt >= 0)
	{	static datum  nkey;
		sendcmd(skt, 'I', 0);
		getreply(skt, 'I', &nkey);
		return nkey;
	}
#endif RMTDBM
	return hashfirstkey(&hashdbm);
}

datum nextkey(key)
datum key;
{
#ifdef RMTDBM
	if (skt >= 0)
	{	static datum  nkey;
		sendcmd(skt, 'N', &key);
		getreply(skt, 'N', &nkey);
		return nkey;
	}
#endif RMTDBM
	return hashnextkey(&hashdbm, key);
}

#ifdef RMTDBM
#define BUFLEN  PBLKSIZ

static sendcmd(skt, cmd, dtm)
int  skt;
int  cmd;
datum *dtm;
{
	int   i, n;
	char  *in, *out;
	static   char  buf[BUFLEN];

	buf[0] = cmd;
	switch (cmd)
	{
		case 'O':
			sprintf(&buf[1], "%s\n", dtm);
			n = strlen(buf);
			break;
		case 'C':
		case 'I':
			sprintf(&buf[1], "\n");
			n = 2;
			break;
		case 'F':
		case 'N':
			sprintf(&buf[1], "%d\n", dtm->dsize);
			n = strlen(buf);
			for (in = dtm->dptr, out = &buf[n], i = 0; i < dtm->dsize; i++)
				*out++ = *in++;
			n += dtm->dsize;
			break;
	}
	return write(skt, buf, n);
}

static getreply(skt, cmd, dtm)
int  skt;
int  cmd;
datum *dtm;
{
	static   char  buf[BUFLEN];
	char  c;
	int   s;
	char *cp = buf;

	errno = 0;
	while ((s = read(skt, &c, 1)) == 1 && c != cmd) continue;
	if (s != 1)
	{	if (dtm) dtm->dsize = 0, dtm->dptr = 0;
		return -1;
	}

	while ((s = read(skt, &c, 1)) == 1 && c != '\n') *cp++ = c;
	*cp = 0;
	if (s != 1)
	{	if (dtm) dtm->dsize = 0, dtm->dptr = 0;
		return -1;
	}

	if (buf[0] != '0')
	{	errno = atoi(&buf[1]);
		if (dtm) dtm->dsize = 0, dtm->dptr = 0;
		return -1;
	}

	if (dtm)
	{	dtm->dsize = atoi(&buf[1]);
		if (read(skt, buf, dtm->dsize) != dtm->dsize)
		{	if (dtm) dtm->dsize = 0, dtm->dptr = 0;
			return -1;
		}
		dtm->dptr = buf;
	}
	return 0;
}
#endif RMTDBM
