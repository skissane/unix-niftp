/* unix-niftp lib/ubc/yb.c $Revision: 5.5 $ $Date: 90/08/01 13:39:05 $ */

/*
 *	A user level implimentation of the Yellow Book Transport Service
 *	calling on the UBC X25 sockets.
 *	The interface presented by this library is in terms of sockets.
 *	It supports only one active socket at a time.
 *	Now with fast select!
 *
 *	wja@uk.ac.nott.cs
 *
 * $Log:	yb.c,v $
 * Revision 5.5  90/08/01  13:39:05  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:55:10  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:55:52  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 * Revision 2.5  86/07/29  02:11:52  bin
 * Catch SIGPIPE filtering out unexpected instances and handing them
 * onto earlier signal handlers
 * 
 * Revision 2.4  86/07/15  20:14:30  bin
 * fix bug: lastread needs clearing for new connections
 * tidy up error reporting so logfile is less alarming
 * 
 * Revision 2.3  86/03/04  01:40:43  bin
 * Nottingham Working Version 3rd March 1986.
 * Merged and tidyed up.
 * 
 * Revision 2.2  85/11/27  23:11:33  bin
 * Nottingham Working Version - Nov 14th 1985.
 * 
 * Revision 1.5  85/08/13  22:40:16  bin
 * Prevent junk characters appearing at end of calling address
 * 
 * Revision 1.4  85/08/09  19:22:34  bin
 * Do not cannonicalise calling address. Let phils stuff do it.
 * Just strip parity bits.
 * 
 * Revision 1.3  85/08/05  02:06:04  wja
 * put in a few more field length checks
 * 
 * Revision 1.2  85/08/05  01:35:13  wja
 * Support both standard and fast select calls
 * 
 * Revision 1.1  85/08/04  22:58:54  wja
 * Initial revision
 * 
 */

#define YBDEBUG(X, Y) if(ftp_print & (X)) printf Y

#include <sys/types.h>
#include <sys/socket.h>
#include <netccitt/x25_sockaddr.h>
#include <netccitt/x25err.h>
#include <signal.h>
#include <errno.h>
#include <setjmp.h>
#include "yb.h"

#define YB_CONNECT	16
#define YB_ACCEPT	17
#define YB_DISCONNECT	18
#define YB_RESET	19
#define YB_ADDRESS	20

#define YBERR_OK	0	/* Successful completion of call */
#define YBERR_DISC	1	/* response to DISCONNECT */
#define YBERR_NOCONN	16	/* no connection (unspecific) */
#define YBERR_BUSY	17	/* number busy */
#define YBERR_OUT	18	/* out of order */
#define YBERR_INV	19	/* invalid address */
#define YBERR_AB	20	/* access barred */
#define YBERR_QOS	21	/* incompatable facilites (QOS) */
#define YBERR_RRC	22	/* no reverse charging */
#define YBERR_NCG	22	/* network congestion */
#define YBERR_LEN	23	/* message too long */
#define YBERR_LPE	32	/* Local procedure error */
#define YBERR_RPE	33	/* Remote procedure error */
#define YBERR_APL	39	/* Application Failure */
#define YBERR_TS	40	/* Transport Service Failure */


#define USERDATA	12 /* bytes of uncommited user data */
#define FASTUSERDATA	124 /* bytes of uncommited user data with fast select*/
#define MAXPACKSIZE	257

#define MBIT		0x40
#define QBIT		0x80

#define LAST_FRAGMENT	0200
#define CONTROL_FOLLOWS 0100
#define FRAGMENT_LENGTH  077

#define NPARAM		5
#define MAXPARAMLEN	256
#define PARAMPLUS	(MAXPARAMLEN + 2) /* some slop for nulls */

#define NOFILE		20 /* from param.h realy, lazy */

/* yb_state values */

#define ST_CLOSED	0	/* unasigned */
#define ST_SOCKET	1	/* got socket open */
#define ST_CONN		2	/* x25 level connection */
#define ST_DATA		3	/* yellow book level connection */

#define NULL		0
#define min(X, Y)	((X)<(Y)?(X):(Y))

/*
 * Character map for address operations
 * the top bit is striped before lookup
 * the output is the official representation of
 * the character
 * All other graphics characters should be seperators, but not
 * just now
 *
 *  YB p26
 */

char addrchars[] = {
	000, 001, 002, 003, 004, 005, 006, 007,
	010, 011, 012, 013, 014, 015, 016, 017,
	020, 021, 022, 023, 024, 025, 026, 027,
	030, 031, 032, 033, 034, 035, 036, 037,
	040, 041, 042, 043, 044, 045, 046, 047,
	050, 051, 052, '.', '.', '.', '.', '.',
	060, 061, 062, 063, 064, 065, 066, 067,
	070, 071, 072, 073, 074, 075, 076, 077,
	100, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', 133, 134, 135, 136, 137,
	140, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', 173, 174, 175, 176, 177,
};

#ifdef TEST_PID
extern char ybpid[];
#else
char ybpid[4] = {0x7f, 0xff, 0xff, 0xff};
#endif

char bigbuf[2048]; /* build cannonical messages here */
	char packbuf[MAXPACKSIZE];
int biglen;
int packlen = 128;
int lastread;
int lastwrite;
char *peekpack;
char peeklen;
int yb_state[NOFILE];

struct param {
	short len;
	char space[PARAMPLUS];
} param[NPARAM];

struct sockaddr_yb ouraddr;
struct sockaddr_yb theiraddr;
char ourspace[3*PARAMPLUS];
char theirspace[3*PARAMPLUS];
int  yb_use_fast_select = 1;
int  yb_pipe_catch();
int  (*yb_origpipe_catch)();
int  ybpipe;
jmp_buf yb_jmp_write;

extern int errno;
extern int ftp_print;

/******************************************************************************
 *									      *
 *			  External interface routines			      *
 *									      *
 ******************************************************************************/

	/*
	 * Clear data structures
	 * return an X25 socket
	 */

yb_socket(af, type, protocol)
	int af, type, protocol;
{
	int fd;
	int (*pipetmp)();

	YBDEBUG(040, ("socket(%d, %d, %d)\n", af, type, protocol));
	initaddr_yb(&ouraddr, ourspace);
	initaddr_yb(&theiraddr, theirspace);
	fd = socket(AF_CCITT, SOCK_STREAM, 0);
	if (fd >= 0)
		yb_state[fd] = ST_SOCKET;
	/* clear various state variables */
	lastread = 0;
	lastwrite = 0;
	/* save old SIGPIPE catcher */
	pipetmp = signal(SIGPIPE, yb_pipe_catch);
	if (pipetmp != yb_pipe_catch)
		yb_origpipe_catch = pipetmp;
	return(fd);
}

	/*
	 * load up calling address
	 * bind the yb pid to the x25 socket
	 */

yb_bind(fd, addr, addrlen)
	int fd;
	struct sockaddr_yb *addr;
	int addrlen;
{
	struct x25_sockaddr sock;

	YBDEBUG(040, ("bind(%s)\n", addr->syb_addr));
	copyaddr_yb(&ouraddr, addr);
	bcopy(ybpid, sock.xaddr_proto, sizeof(sock.xaddr_proto));
	return(bind(fd, &sock, sizeof(sock)));
}

	/*
	 * do listen on x25 socket
	 */

yb_listen(fd, backlog)
	int fd, backlog;
{
	YBDEBUG(040, ("listen(fd, %d)\n", backlog));
	return(listen(fd, backlog));
}

	/*
	 * do accept on x25 socket
	 * clear calls not on our listen string
	 * collect and build return address structure
	 */

yb_accept(fd, addr, addrlen)
	int fd;
	struct sockaddr_yb *addr;
	int *addrlen;
{
	int newfd, n, type;
	struct x25_sockaddr sock;
	int socklen = sizeof(sock);
	struct x25_fastdata fast;
	char addrbuf[PARAMPLUS];
	char userdatabuf[FASTUSERDATA];
	int userdatalen;

	YBDEBUG(040, ("accept()\n"));

	if ((newfd = accept(fd, &sock, &socklen)) < 0) {
		printf("x25 accept failed %d\n", errno);
		return(-1);
	}
	yb_state[newfd] = ST_CONN;

	/* get the user data from the call packet */

	bigbuf[0] = YB_CONNECT;

	fast.xd_fastdata = userdatabuf;
	fast.xd_fastlen = sizeof(userdatabuf);
	if (ioctl(newfd, XIOCGFAST, &fast) < 0 || fast.xd_fastlen == 0) {
		if(fast.xd_fastlen != 0)
			printf("get fast data failed %d\n", errno);
		/* no fast select data, use sockaddr bits */
		bcopy(sock.xaddr_userdata, userdatabuf, USERDATA);
		/* dont know how much of the call packet was present
		 * so set an upper bound
		 */
		userdatalen = USERDATA;
	}
	else
		userdatalen = fast.xd_fastlen;

	if (ftp_print&020000) {
		int i;
		printf("cudf =");
		for (i=0; i < userdatalen; i++) {
			if (i%16 == 15)
				printf("\n");
			printf(" %o", userdatabuf[i] & 0377);
		}
		printf("\n");
	}

	/* peice together the connect packet */
	bigbuf[0] = YB_CONNECT;
	biglen = 1;
	if (userdatabuf[0] & CONTROL_FOLLOWS) {
		/* control packet follows */
		if ((biglen = receivecontrol(newfd, bigbuf + userdatalen,
					sizeof(bigbuf) - userdatalen)) < 0)
			goto disconnect;
	}
	/* glue in userdata - this will overwrite the type byte of
	 * the continuation packet. but we know its connect!
	 */
	bcopy(userdatabuf, bigbuf+1, userdatalen);
	biglen += userdatalen;

	n = yb_decode(&type, param, NPARAM, bigbuf, biglen);

	if (type != YB_CONNECT || n < 1)
		goto disconnect;
	yb_state[newfd] = ST_DATA;

	/*
	 * look at called address
	 */
	ybaddrcan(addrbuf, param[0].space);
	YBDEBUG(2, ("called on %s\n", addrbuf));
	if (!ybaddrmatch(addrbuf, ouraddr.syb_addr)) {
		/* not us */
		printf("Closing call to %s\n", addrbuf);
		goto disconnect;
		}

	/* where do we put this YUCK */
	ouraddr.syb_addrlen = param[0].len;
	strcpy(ouraddr.syb_addr, addrbuf);

	/*
	 *  look at calling address
	 */

	initaddr_yb(&theiraddr, theirspace);
	strcpy(theiraddr.syb_addr, sock.xaddr_addr);
	strcat(theiraddr.syb_addr, ".");
	/* ybaddrcan(addrbuf, param[1].space); for phil */
	parstrip(param[1].space, addrbuf, param[1].len);
	addrbuf[param[1].len] = 0;
	strcat(theiraddr.syb_addr, addrbuf);
	theiraddr.syb_addrlen = strlen(theiraddr.syb_addr);

	/* ignore quality of service */
	theiraddr.syb_qoslen = 0;

	/* explanatory text */

	theiraddr.syb_explen = param[3].len;
	parstrip(param[3].space, theiraddr.syb_exp, param[3].len);

	/*
	 * Send them an accept packet (with zero parrameters)
	 */

	YBDEBUG(020, ("listen: sending accept\n"));

	biglen = yb_encode(YB_ACCEPT, param, 0, bigbuf, sizeof(bigbuf));

	if (sendcontrol(newfd, bigbuf, biglen) < 0)
		goto disconnect;

	/* accept has gone off, so should be ok */
	copyaddr_yb(addr, &theiraddr);
	return(newfd);

disconnect:
	YBDEBUG(1, ("listen: doing disconnect\n"));
	if (yb_state[newfd] == ST_DATA) {
		senddisconnect(newfd, YBERR_NOCONN); /* what should it be? */
		sleep(10);
	}
	close(newfd);
	yb_state[newfd] = ST_CLOSED;
	return(-1);
}

	/*
	 * build call packet
	 * make x25 call
	 * collect yb accept packet
	 */

yb_connect(fd, addr, addrlen)
	int fd;
	struct sockaddr_yb *addr;
	int addrlen;
{
	struct x25_sockaddr sock;
	struct x25_fastdata fast;
	char *ap;
	int i, n, type, userdatalen;
	int moreconnect = 0;

	YBDEBUG(040, ("connect(%s)\n", addr->syb_addr));
	/* make safe copy of their address */
	copyaddr_yb(&theiraddr, addr);

	/*
	 * construct CONNECT message
	 */

	/* build first parameter = Called Address */
		/* split address into active in x25 socket
		 * and dormant in connect message
		 */
	ap = theiraddr.syb_addr;
	sock.xaddr_len = ybaddrfirstcomp(sock.xaddr_addr, 14, &ap);
	strcpy(param[0].space, ap);
	param[0].len = strlen(param[0].space);

	/* build second parameter = Calling Address */
		/* active part is placed in x25 call packet by CPSE
		 * so we just take care of the dormant part
		 */
	param[1].len = ouraddr.syb_addrlen;
	bcopy(ouraddr.syb_addr, param[1].space, ouraddr.syb_addrlen);

	/* build third parameter = Quality of Service */
		/* explicitly set to null */
	param[2].len = 0;

	/* build fourth parameter = Explanatory text */

	param[3].len = theiraddr.syb_explen;
	bcopy(theiraddr.syb_exp, param[3].space, theiraddr.syb_explen);

	/* build cannonical message */

	biglen = yb_encode(YB_CONNECT, param, 4, bigbuf, sizeof(bigbuf));
	biglen -= 1; /* ignore type byte */

	if (biglen > USERDATA && yb_use_fast_select)
		userdatalen = min(FASTUSERDATA, biglen);
	else
		userdatalen = min(USERDATA, biglen);

	/* fast select call */
	if (userdatalen > USERDATA) {
		fast.xd_fastdata = bigbuf + 1;
		fast.xd_fastlen = userdatalen;
		if(biglen > userdatalen) {
			bigbuf[1] |= CONTROL_FOLLOWS;
			moreconnect++;
		}
		if(ioctl(fd, XIOCSFAST, &fast) < 0) {
			printf("set fast select user data failed %d\n", errno);
			userdatalen = min(USERDATA, userdatalen);
		}
	}
	/* standard call */
	if (userdatalen <= USERDATA) {
		if (biglen > userdatalen) {
			bigbuf[1] |= CONTROL_FOLLOWS;
			moreconnect++;
		}
		bcopy(bigbuf+1, sock.xaddr_userdata, userdatalen);
	}
	if (ftp_print&020000) {
		int i;
		printf("cudf =");
		for (i=0; i < userdatalen; i++) {
			if (i%16 == 15)
				printf("\n");
			printf(" %o", bigbuf[i+1] & 0377);
		}
		printf("\n");
	}

	/*
	 * Place call, if sucessfull send rest of packet
	 */

	if (connect(fd, &sock, sizeof(sock)) < 0) {
		if (errno != EXCLEAR)
			printf("x25 connect failed %d\n", errno);
		return(-1);
	}
	yb_state[fd] = ST_CONN;

	if (moreconnect) {
		bigbuf[userdatalen] = bigbuf[0]; /* move up packet type */
		if (sendcontrol(fd, bigbuf + userdatalen,
					biglen - userdatalen + 1) < 0)
					/* unignore type byte */
			goto cleanup;
	}
	/*
	 * read accept/clear packet
	 */

	if ((i = receivecontrol(fd, bigbuf, sizeof(bigbuf))) < 0)
		goto cleanup;

	n = yb_decode(&type, param, NPARAM, bigbuf, i);

	       /*
		* an x25 clear confirm alone will ack a disconnect
		* so we just close and hope their clear arrives
		*/
	if (type != YB_ACCEPT) {
		sleep(10);
		goto cleanup;
	}
	yb_state[fd] = ST_DATA;
	return(0);

cleanup:
	YBDEBUG(1, ("connect: cleanup exit\n"));
	close(fd); /* get rid of connection - misemulation */
	yb_state[fd] = ST_CLOSED;
	return(-1);
}

	/*
	 * send clear packet
	 * close x25
	 */

yb_close(fd)
	int fd;
{
	YBDEBUG(040, ("close()\n"));
	if (yb_state[fd] == ST_DATA) {
		senddisconnect(fd, YBERR_OK);
		sleep(10);
	}
	close(fd);
	yb_state[fd] = ST_CLOSED;
}

	/*
	 * IO semantics on data are identicle to underlying x25.
	 *
	 * Catch and deal with yb control messages
	 */

yb_read(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	int n, i, type;

	YBDEBUG(040, ("read(fd, buf, %d)\n", len));

	if (yb_state[fd] != ST_DATA) {
		printf("yb_read while in state %d\n", yb_state[fd]);
		return(-1);
	}

	if (peekpack) {
		n = min(peeklen, len);
		bcopy(peekpack, buf, n);
		peekpack = NULL;
	}
	else if ((n = read(fd, buf, len)) < 0) {
#ifdef next
		/* network event */
		switch (errno) {
		default:
		}
#endif
		yb_state[fd] = ST_SOCKET;
	     /* printf("x25 read error %d\n", errno); */
		return(errno == EXCLEAR ? 0 : -1);
	}

	if ((lastread&MBIT) && (lastread&QBIT) != (buf[0]&QBIT)) {
		printf("Q bit changed in M sequence\n");
		/* protocol error - should not happen */
		yb_state[fd] = ST_SOCKET;
		return(-1);
	}

	if (buf[0] & QBIT) {
		YBDEBUG(2, ("Got control packet\n"));
		/* control message */
			/* push back first packet */
		peekpack = buf;
		peeklen = n;
		i = receivecontrol(fd, bigbuf, sizeof(bigbuf));
		yb_decode(&type, param, NPARAM, bigbuf, i);
		if (type == YB_DISCONNECT) {
			YBDEBUG(2, ("Disconnect (cause = %d)\n",
				param[0].space[0]&0xff));
			senddisconnect(YBERR_DISC);
			yb_state[fd] = ST_SOCKET;
			return(0);
		}
		else {
			printf("Unsupported control packet Type = %d, Cause = %d\n",
			type, param[0].space[0]&0xff);
			yb_state[fd] = ST_SOCKET;
			return(-1);
		}
	}

	lastread = buf[0];
	/* otherwise it must be ok */
	YBDEBUG(020, ("read = %d\n", n));
	return(n);
}

yb_write(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	int n;

	YBDEBUG(040, ("write(fd, buf, %d)\n", len));

	if (yb_state[fd] != ST_DATA) {
		printf("yb_write called in state %d\n", yb_state[fd]);
		return(-1);
	}

	if ((n = yb_net_write(fd, buf, len)) < 0) {
		/* network event */
		printf("x25 write error %d\n", errno);
#ifdef next
		switch (errno) {
		default:
		}
#endif
		yb_state[fd] = ST_SOCKET;
		return(-1);
	}
	lastwrite = buf[0];
	return(n);
}

       /*
	* Get our address.
	* During a YB connect the called address replaces our address.
	* getsockname will thus return the called address.
	*/

yb_getsockname(fd, addr, addrlen)
	int fd;
	struct sockaddr_yb *addr;
	int *addrlen;
{
	copyaddr_yb(addr, &ouraddr);
}

       /*
	* Interpreter for error codes
	*/

struct errtab {
	int	err;
	char   *mesg;
} yb_errlist[] = {
	EXRESET,	"X25 Reset: call reset",
	EXROUT,		"X25 Reset: out of order",
	EXRRPE,		"X25 Reset: remote procedure error",
	EXRLPE,		"X25 Reset: local procedure error",
	EXRNCG,		"X25 Reset: network congestion",
	EXRXXX,		"X25 Reset: unrecognised cause",
	EXCLEAR,	"X25 Clear: call cleared",
	EXCBUSY,	"X25 Clear: number busy",
	EXCOUT,		"X25 Clear: out of order",
	EXCRPE,		"X25 Clear: remote procedure error",
	EXCRRC,		"X25 Clear: collect call refused",
	EXCINV,		"X25 Clear: invalid call",
	EXCAB,		"X25 Clear: access barred",
	EXCLPE,		"X25 Clear: local procedure error",
	EXCNCG,		"X25 Clear: network congestion",
	EXCNOB,		"X25 Clear: not obtainable",
	EXCXXX,		"X25 Clear: unrecognised cause",
	0,		NULL,
};

char *
yb_error(no)
	int no;
{
	register struct errtab *ep = yb_errlist;
	extern int sys_nerr;
	extern char *sys_errlist[];

	/* look up table first - x25 and yellow book */
	while(ep->mesg && ep->err != no)
		ep++;
	if(ep->mesg)
		return(ep->mesg);
	/* check for standard errors */
	if(0 <= no && no < sys_nerr)
		return(sys_errlist[no]);
	return("Unknown error");
}

/******************************************************************************
 *									      *
 *			       Internal routines			      *
 *									      *
 ******************************************************************************/

senddisconnect(fd, cause)
	int fd, cause;
{
	/* build disconnect packet */

	YBDEBUG(0100, ("senddisconnect\n"));
	if (yb_state[fd] != ST_DATA)
		return;
	param[0].len = 1;
	param[0].space[0] = cause;
	param[1].len = ouraddr.syb_addrlen;
	bcopy(ouraddr.syb_addr, param[1].space, ouraddr.syb_addrlen);

	biglen = yb_encode(YB_DISCONNECT, param, 2, bigbuf, sizeof(bigbuf));

	sendcontrol(fd, bigbuf, biglen);
	yb_state[fd] = ST_SOCKET;
}

       /*
	* Send control message as a qualified complete packet sequence
	*/

sendcontrol(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	char packbuf[MAXPACKSIZE];
	int l, i;

	YBDEBUG(0100, ("sendcontrol(len = %d)\n", len));
	if (yb_state[fd] != ST_DATA && yb_state[fd] != ST_CONN)
		return(-1);

	/* push data if needed */

	if (lastwrite & MBIT) {
		packbuf[0] = (lastwrite & QBIT);
		yb_net_write(fd, packbuf, 1);
		lastwrite = packbuf[0];
	}

	while (len) {
		l = min(len, packlen);
		/* copy into packbuf to gaurentee free byte in front */
		bcopy(buf, packbuf + 1, l);
		packbuf[0] = QBIT;
		if (len > l)
			packbuf[0] |= MBIT;
		if (ftp_print&020000) {
			printf("write Q packet length %d\n", l);
			for (i = 0; i <= l; i++) {
				if ((i%16) == 15)
					printf("\n");
				printf("%o ", packbuf[i]&0377);
			}
			printf("\n");
		}
		if (yb_net_write(fd, packbuf, l+1) < 0) {
			printf("x25 write error %d\n", errno);
			yb_state[fd] = ST_SOCKET;
			return(-1);
		}
		lastwrite = packbuf[0];
		buf += l;
		len -= l;
	}
	return(0);
}


receivecontrol(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	int left = len;
	int l, n, i;

	YBDEBUG(0100, ("receivecontrol()\n"));
	if (yb_state[fd] != ST_DATA && yb_state[fd] != ST_CONN)
		return(-1);


	if (lastread&MBIT) {
		/* should be at end of tdsu */
		printf("Q packet in M sequence\n");
		yb_state[fd] = ST_SOCKET;
		return(-1);
	}

	do {
		if (peekpack) {
			l = min(peeklen, sizeof(packbuf));
			bcopy(peekpack, packbuf, l);
			peekpack = NULL;
		}
		else if ((l = read(fd, packbuf, sizeof(packbuf))) < 0) {
			printf("x25 read error %d\n", errno);
			yb_state[fd] = ST_SOCKET;
			return(-1);
		}
		if ((packbuf[0]&QBIT) == 0) {
			/* got a datapacket */
			if ((lastread&MBIT) == 0) {
				/* not in control so ok */
				return(0);
			}
			yb_state[fd] = ST_SOCKET;
			return(-1);
		}
		if (ftp_print&020000) {
			printf("read Q packet length %d\n", l-1);
			for (i = 0; i < l; i++) {
				if ((i%16) == 15)
					printf("\n");
				printf("%o ", packbuf[i]&0377);
			}
			printf("\n");
		}
		n = min(left, l-1);
		bcopy(packbuf+1, buf, n);
		left -= n;
		buf += n;
		lastread = packbuf[0];
	} while (lastread & MBIT);

	YBDEBUG(020, ("read = %d\n", len - left));
	return(len - left);
}

       /*
	* performs the yellow book parameter encoding
	* (this version assumes len is big enougth
	*/

yb_encode(type, params, nparam, buf, len)
	int type;
	struct param *params;
	int nparam;
	char *buf;
	int len;
{
	register char *p = buf, *s;
	register int i;
	char *last;
	int l;

	YBDEBUG(0100, ("yb_encode(type = %d, n = %d)\n", type, nparam));
	*p++ = type;
	last = p;

	while (nparam) {
		l = params->len;  /* too late to enforce MAXPARAMLEN */
		s = params->space;
		YBDEBUG(0400, ("len = %d, str = %s\n", l, s));
		do {
			i = min(l, 63);
			*p = i;
			if (i == l)
				*p |= LAST_FRAGMENT;
			YBDEBUG(0400, ("header=%o\n", *p&0377));
			p++;
			l -= i;
			while (i--)
				*p++ = *s++;
		} while (l);
		/* note end of non empty parameters */
		if (params->len > 0)
			last = p;
		params++;
		nparam--;
	}
	/* supress trailing empty parameters */
	return(last - buf); /* is this confusing ucl? NO*/
/*	return(p - buf); */
}

       /*
	* decode the parameters to a Yellow book message
	*/

yb_decode(type, params, nparam, buf, len)
	int *type;
	struct param *params;
	int nparam;
	char *buf;
	int len;
{
	register char *p, *s, *end, c;
	register int i, left;
	int header, seen = 0;

	YBDEBUG(0100, ("yb_decode\n"));
	p = buf;
	end = buf + len;
	*type = *p++;

	while (nparam && p < end && *p != 0) {
		s = params->space;
		left = MAXPARAMLEN;
		while (p < end && *p != 0) {
			header = *p++;
			i = (header & FRAGMENT_LENGTH);
			YBDEBUG(0400, ("header = %o, len = %d, s = %.*s\n",
			    header&077, i, i, p));
			while(i--) {
				if (p < end)
					c = *p++;
				else
					c = 0;
				if (left) {
					*s++ = c;
					left--;
				}
			}
			if (header&LAST_FRAGMENT)
				break;
		}
		params->len = s - params->space;
		*s = 0;
		nparam--;
		params++;
		seen++;
	}

	while (nparam--) {
		params->len = 0;
		params++;
	}

	YBDEBUG(0100, ("seen = %d, type = %d\n", seen, *type));
	return(seen);
}


copyaddr_yb(to, from)
	struct sockaddr_yb *to, *from;
{
	int n;

	/* "to" must have space allocated for string components */

	/* Transport service address */
	n = min(to->syb_addrlen, from->syb_addrlen);
	bcopy(from->syb_addr, to->syb_addr, n);
	if(n < to->syb_addrlen)
		to->syb_addr[n+1] = 0;
	to->syb_addrlen = n;
	/* Quality of service */
	n = min(to->syb_qoslen, from->syb_qoslen);
	bcopy(from->syb_qos, to->syb_qos, n);
	if(n < to->syb_qoslen)
		to->syb_qos[n+1] = 0;
	to->syb_qoslen = n;
	/* Explanatory text */
	n = min(to->syb_explen, from->syb_explen);
	bcopy(from->syb_exp, to->syb_exp, n);
	if(n < to->syb_explen)
		to->syb_exp[n+1] = 0;
	to->syb_explen = n;
}


initaddr_yb(addr, space)
	struct sockaddr_yb *addr;
	char *space;
{
	bzero(space, 3*PARAMPLUS);
	addr->syb_addr = space;
	addr->syb_addrlen = PARAMPLUS;
	space += PARAMPLUS;
	addr->syb_qos = space;
	addr->syb_qoslen = PARAMPLUS;
	space += PARAMPLUS;
	addr->syb_exp = space;
	addr->syb_explen = PARAMPLUS;
}

       /*
	* Cannonicalise YBTS addresses
	*/

ybaddrcan(to, from)
	char *to, *from;
{
	while(*from)
		*to++ = addrchars[*from++ & 0177];
	*to = 0;
}

       /*
	*  Do YBTS addresses compare?,
	* FTP.MAIL will match a proto of FTP
	*/

ybaddrmatch(addr, proto)
	char *addr, *proto;
{
	while (*addr && *proto && addrchars[*addr&0177] == addrchars[*proto&0177])
		addr++, proto++;

	if (*proto == 0 && (*addr == 0 || addrchars[*addr&0177] == '.'))
		return(1);
	return(0);
}

       /*
	* copy first component of a YBTS address
	* paddr is positioned after delimeter
	*/

ybaddrfirstcomp(buf, len, paddr)
	char *buf;
	int len;
	char **paddr;
{
	char *ap = *paddr;
	int left = len;

	while (*ap && addrchars[*ap&0177] != '.') {
		if (left > 0) {
			*buf++ = *ap;
			left--;
		}
		ap++;
	}
	*buf = 0; /* the caller should have left space for this */
	if (addrchars[*ap&0177] == '.')
		ap++;
	*paddr = ap;
	return(len - left);
}

parstrip(from, to, len)
	register char *from, *to;
	register int len;
{
	while (len--)
		*to++ = (*from++ & 0177);
}

/*
 *  wrapper for write catching sigpipes
 *
 *  we try to seperate our own SIGPIPE errors from
 *  others elsewhere.
 *  Only sample the pipe handler during socket opens -
 *  Could get clobbered if its changed under our feet
 */
yb_net_write(fd, buf, siz)
	int fd;
	char *buf;
	int siz;
{
	int ret;

	if (setjmp(yb_jmp_write) == 0) {
		ybpipe = 1;
		ret = write(fd, buf, siz);
		ybpipe = 0;
		return(ret);
	}
	else {
		ybpipe = 0;
		errno = EPIPE;
		return(-1);
	}
}

yb_pipe_catch(sig)
	int sig;
{
	if (ybpipe) {
		longjmp(yb_jmp_write, 1);
	}
	else if (yb_origpipe_catch) {
		if (((int)yb_origpipe_catch) & 01) /* SIGIGNORE */
			return;
		yb_origpipe_catch(sig);
	}
	else /* suicide? */ {
		signal(SIGPIPE, SIG_DFL);
		kill(getpid(), SIGPIPE);
	}
}
