#include <stdio.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include "../lib/gen/hash.h"

#define BUFLEN  PBLKSIZ

datum fetch();
datum firstkey();
datum nextkey();

extern errno;

main(argc, argv)
int argc;
char **argv;
{
    int stat;
    datum  key;
    datum  data;
    char  cmd[BUFLEN];
    char  dbuf[BUFLEN];

    errno = 0;
    dup2(0,1);
    setreuid (1, 1);

    while (getcmd (0, cmd, BUFLEN))
    {
	switch (cmd[0])
	{
	    case 'O':
		sscanf (&cmd[1], "%s", dbuf);
		stat = dbminit (dbuf);
		if (stat >= 0)
		    sendreply (0, cmd[0], 0, 0);
		else
		    goto bad;
		break;

	    case 'C':
		stat = dbmclose ();
		if (stat >= 0)
		    sendreply (0, cmd[0], 0, 0);
		else
		    goto bad;
		break;

	    case 'F':
		sscanf (&cmd[1], "%d", &key.dsize);
		key.dptr = dbuf;
		read (0, dbuf, key.dsize);
		data = fetch (key);
		if (data.dptr != NULL)
		    sendreply (0, cmd[0], data.dsize, data.dptr);
		else
		    goto bad;
		break;

	    case 'I':
		key = firstkey ();
		if (key.dptr != NULL)
		    sendreply (0, cmd[0], key.dsize, key.dptr);
		else
		    goto bad;
		break;

	    case 'N':
		sscanf (&cmd[1], "%d", &key.dsize);
		key.dptr = dbuf;
		read (0, dbuf, key.dsize);
		key = nextkey (key);
		if (key.dptr != NULL)
		    sendreply (0, cmd[0], key.dsize, key.dptr);
		else
		    goto bad;
		break;

	    case '\n':
		break;

	    default:
		errno = 0;
	    bad:
		senderror (0, cmd[0], errno);
		break;
	}
    }
    exit (0);
}

getcmd (fd, buf, len)
int   fd;
char *buf;
int   len;
{
    char  c;
    int   s;
    char *cp = buf;

    while ((s = read (fd, &c, 1)) == 1 && c != '\n' && --len > 0)
	*cp++ = c;
    *cp = 0;
    return (s);
}

sendreply (fd, cmd, len, data)
int   fd;
int   cmd;
int   len;
char *data;
{
    char  buf[BUFLEN+16];
    int   i, n;
    char  *in, *out;

    buf[0] = cmd; buf[1] = '0';
    sprintf (&buf[2], "%d\n", len);
    n = strlen (buf);
    for (in = data, out = &buf[n], i = 0; i < len; i++)
	*out++ = *in++;
    return (write (fd, buf, n + len));
}

senderror (fd, cmd, err)
int   fd;
int   cmd;
int   err;
{
    char  buf[128];
    int   n;

    buf[0] = cmd; buf[1] = 'E';
    sprintf (&buf[2], "%d\n", err);
    n = strlen (buf);
    return (write (fd, buf, n));
}
