/* X25 over a boring stream (in this case TCP) */

/*
 *	an implementation of X.25 over a boring stream (e.g. tcp/ip)
 *
 *	Copyright (c) Piete Brooks 1987
 */

#ifndef	lint
static char RCSid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/x25b/x25b.c,v 5.5 90/08/01 13:39:25 pb Exp $";
static char *_versions	= "$Revision: 5.5 $";
static char *_dates	= "$Date: 90/08/01 13:39:25 $";
#endif	lint

/*
 *	This implements the x25bridge protocol (x25 over TCP/IP)
 *
 *	The client process should provide two routines:
 *
 *	x25b_logit(mask, format, arg1, arg2, arg3, arg4);
 *	int	mask;
 *	char	*format;
 *	{	fprintf(stderr, format, arg1, arg2, arg3, arg4);
 *	}
 *
 *	x25b_perror(mask, format, arg1, arg2, arg3, arg4);
 *	int	mask;
 *	char	*format;
 *	{	fprintf(stderr, format, arg1, arg2, arg3, arg4);
 *		fflush(stderr);
 *		perror("");
 *	}
 *
 *	Note that perror will need some sort of newline added at the end.
 *
 *	The bits in n are:
 *	
 *	X25B_L_STAMP		this is a continuation of the previous msg
 *	X25B_L_STAMP		if possible, datestamp this message
 *	X25B_L_DEBUG		this is a debug message (may be ignored)
 *
 *
 *	The interface is
 *	bytes = x25b_read_data(fd, p_send_type, buff, len)
 *	bytes = x25b_read_data2(fd, p_send_type, buff, len,
 *		p_flags, p_tspad, p_x25io)
 *	char *p_send_type;
 *	char *buff;
 *
 *	Read up to len bytes into buff, and put the X25 QMD bits into
 *	*p_send_type.
 *	If no data is read, *p_send_type is set to 0xff.
 *	If p_send_type is NULL, it is not used.
 *	*** Excess data is DISCARDED ***
 *	if buff == p_x25io->x_buf then p_x25io is used directly,
 *	otherwise the data is copied into an internal structure.
 *
 *
 *	bytes = x25b_write_data2(fd, p_send_type, buff, len,
 *		p_flags, p_tspad, p_x25io)
 *	bytes = x25b_write_data(fd, p_send_type, buff, len)
 *	char *p_send_type;
 *	char *buff;
 *	char *p_flags;
 *	char *p_tspad;
 *	char *p_x25io;
 *
 *	Write len bytes from buff, using *p_send_type as the QMD bits.
 *	If flags is NULL, the bits are assumed to be zero.
 *
 *
 *	x25b_open_server(server, port)
 *	char *server;
 *	char *port;
 *	server is a comma separated list of gateways to try.
 *	If NULL or a null string, use the compiled in default (x25-serv)
 *	port is the tcp port number (numeric or as in /etc/services)
 *	defaulting to spad (yuck!)
 *	These are overridden by environment variables X25SERVER and X25PORT.
 *
 *
 *	x25b_open_ybts4(dte, ybts, callingdte, callingname, callingybts,
 *			server, port, user, facil, x25iop)
 *	char *dte;
 *	char *ybts;
 *	char *callingdte;	[ 1+ ]
 *	char *callingname;	[ 3+ ]
 *	char *callingybts;
 *	char *server;
 *	char *port;
 *	char *user;		[ 4+ ]
 *	struct facilities *facil;
 *	struct x25io *x25iop;
 *
 *	x25b_open_x29_2(dte, xcudf, callingdte, server, port,
 *		user, facil, x25iop)
 *	char *dte;
 *	char *xcudf;
 *	char *callingdte;
 *	char *server;
 *	char *port;
 *	char *user;		[ 2+ ]
 *	struct facilities *facil;
 *	struct x25io *x25iop;
 *
 *	The third form of open .....
 *	the first two specify the address to be called.
 *	cudf may be:
 *		<text>
 *		:<hex>
 *		::<pid><hex>
 */

#define	FULL_X25STR
#include "x25b.h"
#include <errno.h>		/* EWOULDBLOCK */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>

struct x25io _x25io;
struct servent *getservbyname();
char *getenv();
char *index();
struct hostent *gethostbyname();

x25b_read_data(fd, p_send_type, buff, len)
char *p_send_type;
char *buff;
{	return x25b_read_data2(fd, p_send_type, buff, len, 0, 0, 0);
}

x25b_read_data2(fd, p_send_type, buff, len, p_flags, p_tspad, p_x25io)
char *p_send_type;
char *buff;
char *p_flags;
char *p_tspad;
struct x25io *p_x25io;
{	int n;

	if (! p_x25io) p_x25io = &_x25io;

	if ((n = _x25b_readpkt(fd, p_x25io)) <= 0)
	{	if (p_send_type) *p_send_type = 0xff;
		return n;
	}

	if (p_x25io->x_flags == X25F_SBUF ||
		p_x25io->x_flags == 0xd2 || p_x25io->x_flags == 0x61)
	{	if (p_x25io->x_flags == 0x61)
		{	p_x25io->x_flags = 0xd2;
		}
		_x25b_log_buff(fd, (char *) p_x25io, n+6, 0);
		send(fd, (char *) p_x25io, n+6, 0);
		n -= (((char *) (&p_x25io->x_sbuf)) -  (char *) p_x25io);
#define	STRUCT_FUDGE	(-2)
		if (p_x25io->x_flags == 0xd2 || p_x25io->x_flags == 0x61)
		{	x25b_logit(X25B_L_STAMP,
				"Sigh -- spad set flags wrong\n");
			p_x25io->x_flags = X25F_SBUF;
			p_x25io->x_send_type = 0;
		}
		if (n > len)
		{	x25b_logit(X25B_L_STAMP,
				"I had %d was only asked for %d\n",
				n, len);
			n = len;
		}
		x25b_logit(0, "[OffSet: %d, %d]",
			((char *)  p_x25io->x_buf) - (char *) p_x25io,
			((char *) (&p_x25io->x_sbuf)) - (char *) p_x25io);

		_x25b_print_facil(p_x25io->x_facil);
		if (p_send_type) *p_send_type = p_x25io->x_send_type;
		bcopy(&((char *)(&p_x25io->x_sbuf))[STRUCT_FUDGE], buff, n);
		return n;
	}
	if (p_send_type) *p_send_type = p_x25io->x_send_type;
	if (p_tspad) *p_tspad = p_x25io->x_tspad;
	if (p_flags) (*p_flags = p_x25io->x_flags);
	if (buff == p_x25io->x_buf)
		;
	else if (p_x25io->x_flags & X25F_TSPAD)
	{	*buff = p_x25io->x_tspad;
		bcopy(p_x25io->x_buf, buff+1, n);
		n++;
	}
	else bcopy(p_x25io->x_buf, buff, n);
	return n;
}

x25b_write_data(fd, p_send_type, buff, len)
char *p_send_type;
char *buff;
{	return x25b_write_data2(fd, p_send_type, buff, len, 0, 0, 0);
}

x25b_write_data2(fd, p_send_type, buff, len, p_flags, p_tspad, p_x25io)
char *p_send_type;
char *buff;
char *p_flags;
char *p_tspad;
struct x25io *p_x25io;
{	static int skip = 0;
	int slen = x25hdrsize + len + 1 + TS29_BYTES;
	char *data;
	int rc;
	extern errno;

	if (!p_x25io) p_x25io = &_x25io;

	data = (char *) p_x25io;
	if (buff != p_x25io->x_buf)	bcopy(buff, p_x25io->x_buf, len);

	if (p_x25io->x_version != X25IO_VER)
	{	x25b_logit(X25B_L_STAMP, "write data: version was %d (%d)\n",
				 p_x25io->x_version, X25IO_VER);
		p_x25io->x_version = X25IO_VER;
	}
	p_x25io->x_tspad	= (p_tspad) ? *p_tspad : 0;
	p_x25io->x_flags	= (p_flags) ? *p_flags : 0;
	p_x25io->x_send_type	= (p_send_type) ? *p_send_type : 0;
	p_x25io->x_count_ms	= len / 256;
	p_x25io->x_count_ls	= len % 256;

	if (x25b_debug & 2) x25b_logit(1, "wd%3d [%08x %08x %08x]\n",
		len,
		((long *) p_x25io)[0],
		((long *) p_x25io)[1],
		((long *) p_x25io)[2]);
	_x25b_log_buff(fd, (char *) p_x25io, slen, 0);
	if (skip)
	{	data += skip, slen -= skip;
		x25b_logit(1, "Skip %d on %08x %08x %08x\n",
			skip,
			((long *) p_x25io)[0],
			((long *) p_x25io)[1],
			((long *) p_x25io)[2]);
	}
	errno = -1;
	rc=send(fd, data, slen, 0);
	if (rc != slen)
	{	x25b_logit(1, "Old  %d on %08x %08x %08x ",
			skip,
			((long *) p_x25io)[0],
			((long *) p_x25io)[1],
			((long *) p_x25io)[2]);
		skip += (rc < 0) ? 0 : rc;
		x25b_logit(1, "%d -> %d so %d (%d)\n", rc, slen, skip, errno);
		errno = EWOULDBLOCK;
		return -1;
	}
	else skip = 0;

	return (rc <= 0) ? -1 : len;
}

x25b_open_server(servers, port)
char *servers;
char *port;
{	struct sockaddr_in to;
	struct hostent *gethostent(), *host;
	int portn;
	char *next;
	char *env;
	int fd = -1;

	if (env = getenv("X25PORT")) port = env;
	if (!port || !*port) port = "spad";

	if (env = getenv("X25SERVER")) servers = env;
	if (!servers || !*servers) servers = SERVERNAME;

	if (isdigit(*port)) portn = htons(atoi(port));
	else
	{	struct servent *sp = getservbyname(port, "tcp");
		if (!sp)
		{	x25b_logit(X25B_L_STAMP, "no %s/tcp service\n", port);
			return -1;
		}
		portn = sp->s_port;
	}

	/* loop to try multiple servers */
	for(next=servers; next; )
	{	char server[128];
		char *this = next;

		next = index(this, ',');
		strcpy(server, this);
		if (next) server[next++ - this] = '\0';
		if (x25b_debug & 4) x25b_logit(X25B_L_STAMP, "Try %s (%s|%s)",
			server, this, servers);

		host = gethostbyname(server);
		if (!host) {
			x25b_logit(X25B_L_STAMP,
				" +++- can't find the server %s\r\n", server);
			continue;
		}
		bzero((char *)&to, sizeof(to));
		bcopy(host->h_addr, (char *)&to.sin_addr, host->h_length);
		to.sin_family = host->h_addrtype;
		to.sin_port = portn;
		fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0)
		{	x25b_logit(X25B_L_STAMP, " +++- %s: ", server);
			x25b_perror(X25B_L_STAMP, "client socket");
			continue;
		}
		if (connect(fd, (struct sockaddr *) &to, sizeof(to)) < 0)
		{	x25b_logit(X25B_L_STAMP, " +++- %s: (%s/%s=%d): ",
				server, port, port, portn);
			x25b_perror(X25B_L_STAMP, "client connect");
			close(fd);
			fd = -1;
			continue;
		}
		if (x25b_debug & 4) x25b_logit(X25B_L_STAMP | X25B_L_DEBUG,
				"++ call %s on port 0x%x\n", server, portn);
		break;
	}
	if (!host)
	{	x25b_logit(X25B_L_STAMP,
			" +++- can't find the servers (%s)\n", servers);
		return -1;
	}
	if (fd < 0)
	{	x25b_perror(X25B_L_STAMP, "cant find a respondng server");
		return -1;
	}

	if (x25b_debug & 4) x25b_logit(X25B_L_STAMP | X25B_L_DEBUG,
		"Returning %d\n", fd);
	return fd;
}

_x25b_init_x25io(x25io)
struct x25io *x25io;
{	bzero((char *)x25io, sizeof(struct x25io));
	x25io->x_version	= X25IO_VER;
}

_x25b_make_connection(fd, x25iop)
struct x25io *x25iop;
{
	if (!x25iop)	x25iop = &_x25io;

	/* Now make up the data part of the packet */
	x25iop->x_tspad		= 0;
	x25iop->x_ssize		= x25sbuf_size;
	x25iop->x_flags		= X25F_SBUF;
	x25iop->x_send_type	= 0;
	x25iop->x_count_ms	= x25SBUF_size / 256;
	x25iop->x_count_ls	= x25SBUF_size % 256;

	_x25b_log_buff(fd, (char *) &_x25io, x25hdrsize + x25SBUF_size+1+TS29_BYTES, 0);
	if (send(fd, (char *) x25iop,
		x25hdrsize + x25SBUF_size + 1 + TS29_BYTES, 0) < 0)
	{	close(fd);
		x25b_perror(X25B_L_STAMP, "Initial send failed");
		return -1;
	}

	while(1)
	{	int count;
		count = _x25b_readpkt(fd, x25iop);
		if (x25b_debug & 4) x25b_logit(X25B_L_STAMP | X25B_L_DEBUG,
		   "read_pkt(%d) gave %d %02x\n", fd, count, x25iop->x_flags);
		if(x25iop->x_flags &	X25F_SBUF && count == x25SBUF_size)
									break;
		if(x25iop->x_flags &	X25F_GATE_MSG)
		{	x25b_logit(X25B_L_STAMP,
				"\r\n ++no call yet-- %s\r\n", x25iop->x_buf);
			if (x25iop->x_flags &	X25F_CLOSING)
			{	close(fd);
				return -1;
			}
			continue;
		}
		x25b_perror(X25B_L_STAMP, "client readback of setup info");
		close(fd);
		return -1;
	}
	if (x25b_debug & 4) x25b_logit(X25B_L_STAMP | X25B_L_DEBUG,
		"Return %d\n", fd);
	return fd;
}

_x25b_readpkt(fd, x25iop)
struct x25io *x25iop;
{	int count, acount;

	count = _x25b_recvfill(fd, (char *)x25iop, x25hdrsize, 0);
	if (count != x25hdrsize)
	{	if (count <= 0)		return count;
		else
		{	x25b_logit(X25B_L_STAMP,
			  "client recv: short delivery - wanted %d, got %d\n",
				 x25hdrsize, count);
			return TTY_NODATA;
		}
	}

	if (x25iop->x_version != X25IO_VER)
	{	x25b_logit(X25B_L_STAMP,
				"client recv: Invalid version %d/%x (%d)\r\n",
				x25iop->x_version, 
				*((long *) x25iop),
				X25IO_VER);
		return -1;
	}

	count = (x25iop->x_count_ms << 8) + x25iop->x_count_ls +1+TS29_BYTES;
	acount = _x25b_recvfill(fd, x25iop->x_rawbuf, count, 0);

	if (count != acount)
	{	if (acount <= 0)	return count;
		else
		{	x25b_logit(X25B_L_STAMP,
		      "client recv B: short delivery - wanted %d, got %d\r\n",
			 count, acount);
			return -1;
		}
	}
		
	return count-1-TS29_BYTES;
}


_x25b_recvfill(sock, buf, count, flags) /* recv as <count> bytes from sock */
     char *buf;
{
	int acount, total;
	char *bufp = buf;
	total = 0;
	if (x25b_debug & 2) x25b_logit(1, "rf%3d ", count);
	while (total < count) {
		acount = recv(sock, bufp, (count - total), flags);
		if (x25b_debug & 2) if (acount != count)
				x25b_logit(1, "%3d ", acount);
		_x25b_log_buff(~sock, bufp, acount, flags);
		if (acount <= 0)
		{	if (x25b_debug & 2) x25b_logit(1, "=%3d %3d [%08x %08x %08x]\n",
				count, acount,
				((long *)buf)[0],
				((long *)buf)[1],
				((long *)buf)[2]);
			return(acount);		/* report any anomoly */
		}
		bufp += acount;
		total += acount;
	}
	if (x25b_debug & 2) if (acount != count) x25b_logit(1, "=%3d ",count);
	if (x25b_debug & 2) x25b_logit(1, "[%08x %08x %08x]\n",
		((long *)buf)[0],
		((long *)buf)[1],
		((long *)buf)[2]);
	return(count);
}

#define FAC_x4_fflags	ntohs(facil->x4_fflags)
_x25b_print_facil(facil)
struct facilities *facil;
{	x25b_logit(X25B_L_STAMP, " +++- ");
	if (FAC_x4_fflags & (FACIL_F_REVERSE_CHARGE))
		x25b_logit(0, "%srev chge, ",
			(facil->x4_reverse_charge) ? "no " : "");
	if (FAC_x4_fflags & (FACIL_F_RECVPKTSIZE | FACIL_F_SENDPKTSIZE))
		x25b_logit(0, "pkt=%d/%d, ",
		ntohs(facil->x4_recvpktsize), ntohs(facil->x4_sendpktsize));
	if (FAC_x4_fflags & (FACIL_F_RECVWNDSIZE | FACIL_F_SENDWNDSIZE))
		x25b_logit(0, "wnd=%d/%d, ",
			facil->x4_recvwndsize,	facil->x4_sendwndsize);
	if (FAC_x4_fflags & (FACIL_F_RECVTHRUPUT | FACIL_F_SENDTHRUPUT))
		x25b_logit(0, "thru=%d/%d, ",
			facil->x4_recvthruput, facil->x4_sendthruput);
	if (FAC_x4_fflags & FACIL_F_CUG_INDEX)
		x25b_logit(0, "cug=%x, ", facil->x4_cug_index);
	if (FAC_x4_fflags & FACIL_F_FAST_SELECT) x25b_logit(0, "%s, ",
		(facil->x4_fast_select == FACIL_NO)	? "FS off" :
		(facil->x4_fast_select == FACIL_FCS_CLR)? "FS CLR" :
		(facil->x4_fast_select == FACIL_YES)	? "FS ACC" : "??");
	if (FAC_x4_fflags & FACIL_F_RPOA)
		x25b_logit(0, "rpoa %04x, ", ntohs(facil->x4_rpoa));
	x25b_logit(0, "\r\n");
}

/* The first interface (of many) to open an outgoing Yellow-Book call */

x25b_open_ybts(dte, ybts, callingybts, server, port, facil, x25iop)
char *dte;
char *ybts;
char *callingybts;
char *server;
char *port;
struct facilities *facil;
struct x25io *x25iop;
{	return x25b_open_ybts4(dte, ybts, (char *) 0, (char *) 0, callingybts,
		server, port, (char *) 0, facil, x25iop);
}

x25b_open_ybts2(dte, ybts, callingdte, callingybts, server, port, facil, x25iop)
char *dte;
char *ybts;
char *callingdte;
char *callingybts;
char *server;
char *port;
struct facilities *facil;
struct x25io *x25iop;
{	return x25b_open_ybts4(dte, ybts, callingdte, (char *) 0, callingybts,
		server, port, (char *) 0, facil, x25iop);
}

x25b_open_ybts3(dte, ybts, callingdte, callingybts, server, port, user, facil, x25iop)
char *dte;
char *ybts;
char *callingdte;
char *callingybts;
char *server;
char *port;
char *user;
struct facilities *facil;
struct x25io *x25iop;
{	return x25b_open_ybts4(dte, ybts, callingdte, (char *) 0, callingybts,
		server, port, user, facil, x25iop);
}

x25b_open_ybts4(dte, ybts, callingdte, callingname, callingybts,
		server, port, user, facil, x25iop)
char *dte;
char *ybts;
char *callingdte;
char *callingname;
char *callingybts;
char *server;
char *port;
char *user;
struct facilities *facil;
struct x25io *x25iop;
{	int fd;
	char *slash = (dte) ? index(dte, '/') : (char *) 0;

	if (!x25iop)	x25iop = &_x25io;

	if ((fd = x25b_open_server(server, port)) < 0)
	{	x25b_logit(X25B_L_STAMP, "open_server failed\n");
		return fd;
	}

	/* Reset the buffer */
	_x25b_init_x25io(x25iop);

	if (slash)
	{	*slash = '\0';
		if (!ybts || !*ybts) ybts = slash+1;
	}

	if (!ybts) ybts = DEF_YBTS;
	if (!callingybts) callingybts = DEF_YBTS;

	/* Put in the YBTS string */
	bcopy(TS_CUDF, x25iop->x_cudf, TS_CUDFLEN);
	sprintf(x25iop->x_cudf + TS_CUDFLEN, "%c%s%c%s",
		0x80 | strlen(ybts), ybts,
		0x80 | strlen(callingybts), callingybts);
	x25iop->x_cudflen = strlen(x25iop->x_cudf + TS_CUDFLEN) +
			TS_CUDFLEN;

	/* Called address */
	if (callingname || callingdte || !dte)
	    sprintf(x25iop->x_destination, "%c%s%c%s%c%s",
		0x80 + ((dte) ? strlen(dte) : 0), dte ? dte : "",
		0x80 + ((callingdte) ? strlen(callingdte) : 0),
			callingdte ? callingdte : "",
		0x80 + ((callingname) ? strlen(callingname) : 0),
			callingname ? callingname : "");
	else
	    sprintf(x25iop->x_destination, dte);
	strcpy(x25iop->x_username, (user && *user) ? user : "unix-nif");

	/* I assume we want fast call select ... */
	if (facil) bcopy(facil, &(x25iop->x_facil), sizeof (x25iop->x_facil));
	else
	{	bzero(&(x25iop->x_facil), sizeof (x25iop->x_facil));
		x25iop->x_fast_select	= FACIL_FCS;
		x25iop->x_flags		|= FACIL_F_FAST_SELECT;
	}

	if (slash) *slash = '/';

	return _x25b_make_connection(fd, x25iop);
}

x25b_open_x29(dte, xcudf, callingdte, server, port, user, facil, x25iop)
char *dte;
char *xcudf;
char *callingdte;
char *server;
char *port;
char *user;
struct facilities *facil;
struct x25io *x25iop;
{	return x25b_open_x29_2(dte, xcudf, callingdte, (char *) 0,
		server, port, user, facil, x25iop);
}

x25b_open_x29_2(dte, xcudf, callingdte, callingname,
	server, port, user, facil, x25iop)
char *dte;
char *xcudf;
char *callingdte;
char *callingname;
char *server;
char *port;
char *user;
struct facilities *facil;
struct x25io *x25iop;
{	int fd;
	int base = 0;
	char *colon = (char *) 0;

	if (!x25iop)	x25iop = &_x25io;

	if ((fd = x25b_open_server(server, port)) < 0)
	{	x25b_logit(X25B_L_STAMP, "open_server failed\n");
		return fd;
	}

	/* Reset the buffer */
	_x25b_init_x25io(x25iop);

	if (dte && (colon = index(dte, ':')))
	{	*colon = '\0';
		if (!xcudf || !*xcudf || !strcmp(xcudf, ":"))
			xcudf = colon+1;
	}

	if (xcudf && *xcudf != ':')	/* Is this really hex or text ? */
	{	bcopy(PRE_CUDF, x25iop->x_cudf, PRE_CUDFLEN);
		base += PRE_CUDFLEN;
		strcpy(x25iop->x_cudf + base, xcudf);
		base += strlen(xcudf);
	}
	else if (xcudf)	/* need to prefix CUDF with PID ? */
	{	if (*++xcudf == ':') xcudf++;
		else if (strncmp(xcudf, "01", 2))
		{	bcopy(PRE_CUDF, x25iop->x_cudf, PRE_CUDFLEN);
			base += PRE_CUDFLEN;
		}
		_x25b_chartohex(x25iop->x_cudf + base, xcudf);
		base += strlen(xcudf)/2;
	}
	x25iop->x_cudflen = base;

	/* Called address */
	if (callingname || callingdte || !dte)
	    sprintf(x25iop->x_destination, "%c%s%c%s%c%s",
		0x80 + ((dte) ? strlen(dte) : 0), dte ? dte : "",
		0x80 + ((callingdte) ? strlen(callingdte) : 0),
			callingdte ? callingdte : "",
		0x80 + ((callingname) ? strlen(callingname) : 0),
			callingname ? callingname : "");
	else
	    sprintf(x25iop->x_destination, dte);
	strncpy(x25iop->x_username, (user) ? user : "unix-nifp",
		sizeof x25iop->x_username);
	x25iop->x_username[sizeof x25iop->x_username -1] = '\0';

	/* I assume we want fast call select ... */
	if (facil) bcopy(facil, &(x25iop->x_facil), sizeof (x25iop->x_facil));
	else
	{	bzero(&(x25iop->x_facil), sizeof (x25iop->x_facil));
		x25iop->x_fast_select	= FACIL_FCS;
		x25iop->x_flags		|= FACIL_F_FAST_SELECT;
	}

	if (colon) *colon = ':';

	return _x25b_make_connection(fd, x25iop);
}

x25b_receive_call(fd, x25iop)
struct x25io *x25iop;
{	int n;
	_x25b_init_x25io(&_x25io);
	_x25b_init_x25io(x25iop);

	if ((n = _x25b_readpkt(fd, x25iop)) <= 0) return n;

	if (x25iop->x_flags == X25F_SBUF ||
		x25iop->x_flags == 0xd2 || x25iop->x_flags == 0x61)
	{	if (x25iop->x_flags == 0x61)
		{	x25iop->x_flags = 0xd2;
		}
		x25iop->x_version	= X25IO_VER;
		_x25b_log_buff(fd, (char *) x25iop, n+6, 0);
		send(fd, (char *) x25iop, n+6, 0);
		n -= (((char *) &x25iop->x_sbuf) - (char *) x25iop);
#define	STRUCT_FUDGE	(-2)
		if (x25iop->x_flags == 0xd2 || x25iop->x_flags == 0x61)
		{	x25b_logit(X25B_L_STAMP,
				"Sigh -- spad set flags wrong\n");
			x25iop->x_flags = X25F_SBUF;
			x25iop->x_send_type = 0;
		}
		x25b_logit(0, "[Offset: %d, %d]",
			((char *)  x25iop->x_buf) - (char *) x25iop,
			((char *) &x25iop->x_sbuf) - (char *) x25iop);

		_x25b_print_facil(&x25iop->x_facil);
		return n;
	}
	return -1;
}

_x25b_log_buff(fd, buff, len, type)
char *buff;
{	extern errno;
	static last_was_hdr=0;

	if (x25b_debug & 0x10)
	{	int i=0;
		int b1=x25hdrsize;
		int b2=b1+2;

		if (last_was_hdr)
		{	b1 = 0;
			b2 = b1 + 2;
		}

		x25b_logit(X25B_L_STAMP | X25B_L_DEBUG,
			"Fd %d %s, Type %02x, Data %d:",
			(fd < 0) ? ~fd : fd,
			(fd < 0) ? "recv" : "send",
			type, len);
		for (; i<b1 && i<len; i++)
		x25b_logit(X25B_L_DEBUG,  "%s%02x",
			(i % 26 == 13) ? "\r\n++ ":" ", 
			((unsigned char *)buff)[i]);
		if (b1 && i<len) x25b_logit(X25B_L_DEBUG,  " -");
		for (; i<b2 && i<len; i++)
		x25b_logit(X25B_L_DEBUG,  "%s%02x",
			(i % 26 == 13) ? "\r\n++ " : " ", 
			((unsigned char *)buff)[i]);
		if (b2 && i<len) x25b_logit(X25B_L_DEBUG,  ":");
		for (; i<len; i++)
		x25b_logit(X25B_L_DEBUG,  "%s%02x",
			(i % 26 == 12) ? "\r\n++ ":" ", 
			((unsigned char *)buff)[i]);
		if (len == b1)	last_was_hdr=1;
		else		last_was_hdr=0;
		x25b_logit(X25B_L_DEBUG,  " (%d)\r\n", errno);
	}
}

_x25b_chartohex(to, from)
char *from;
char *to;
{	int offset = 0;

	for (offset=0; *from; from++, offset++)
	{	int	val;
		switch(*from)
		{	case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			val = (*from) - '0';				break;
			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':
			val = (*from) - 'a' + 10;			break;
			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
			val = (*from) - 'A' + 10;			break;
			default:
			return;
		}

		if (offset & 1)
			to[offset/2] |= val;
		else	to[offset/2]  = val << 4;
	}
}
