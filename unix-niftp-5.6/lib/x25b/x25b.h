#ifndef	X25IO_VER
#define	X25IO_VER	6	/* Current version of x25io	*/
#endif	X25IO_VER

#define	TS29_BYTES	1
#define	X25LOG_VER	1	/* Current version of x25log	*/
#define	TTY_NODATA	-123	/* totty(TTY_NODATA) does nothing. */
#define	NOT_CHAR	(0x100)	/* Not a valid char */
#define	MAXNOFS_BYTES	16	/* Max user data in non FS call	*/
#define	CONNECT		0x10	/* CONNECT header for non FS	*/
/* these constants should be imported from SUN include files - watch out
   for problems at new version time! */

#ifndef	MAXDATA
#define MAXDATA 102		/* 128 - 12 - 4 - 10  */
#define Q_BIT 3
#define D_BIT 2
#define M_BIT 1
#define	MAXHOSTADR	15	/* Max length of DTE [length = 1 nibble] +1 */
#define	INT_DATA	30
#define	VC_RESET	32
#endif	MAXDATA

/* structures for passing x25messages over TCP channel
 * (between client and server)
 * - x25in's are read from the x25 port, sent by the server,
 * received by the client
 */

#define MAXCUDF MAXDATA

/* Now lets check that everything is the same on all machines .... */
#if	MAXHOSTADR != 15
Invalid value for MAXHOSTADR
#endif
#if	MAXCUDF != 102
Invalid value for MAXCUDF
#endif

#define	X25F_MSG_OOB		0x01	/* Is OOB data */
#define	X25F_TSPAD		0x02	/* TS pad character used */
#define	X25F_GATE_MSG		0x04	/* This is a gateway message */
#define	X25F_CLOSING		0x08	/* The gateway is closing */
#define	X25F_SBUF		0x10	/* This is an sbuf */
extern int x25b_debug;

/* Requires X25 parameters	*/
#define	FACIL_YES	1
#define	FACIL_NO	0
#define	FACIL_FCS	FACIL_YES
#define	FACIL_FCS_CLR	2

#define	FACIL_F_RECVPKTSIZE	0x0001
#define	FACIL_F_SENDPKTSIZE	0x0002
#define	FACIL_F_RPOA		0x0004
#define	FACIL_F_REVERSE_CHARGE	0x0008
#define	FACIL_F_RECVWNDSIZE	0x0010
#define	FACIL_F_SENDWNDSIZE	0x0020
#define	FACIL_F_RECVTHRUPUT	0x0040
#define	FACIL_F_SENDTHRUPUT	0x0080
#define	FACIL_F_CUG_INDEX	0x0100
#define	FACIL_F_FAST_SELECT	0x0200

#define XCONTEXT        1       /* NRS contexts:- X29 context */
#define TCONTEXT        2       /* ts29 context */
#define FCONTEXT        3       /* ftp context */
#define MCONTEXT        4       /* mail context */
#define JCONTEXT        7       /* jtmp context */
#define ICONTEXT        12      /* Information context */
#define NCONTEXT	127	/* Non NRS contexts:- news context */
	
struct facilities {
      unsigned	short	x4_fflags;
      unsigned	short	x4_recvpktsize;		/* 16 -> 1024 */
      unsigned	short	x4_sendpktsize;		/* NETWORK ORDER */
      unsigned	short	x4_rpoa;
      unsigned	char	x4_reverse_charge;
      unsigned	char	x4_recvwndsize;
      unsigned	char	x4_sendwndsize;
      unsigned	char	x4_recvthruput;
      unsigned	char	x4_sendthruput;
      unsigned	char	x4_cug_index;		/* 0x00 ~ 0x99 (BCD) */
      unsigned	char	x4_fast_select;		/* NO, FCS, FCS_CLR */
#if	(X25IO_VER > 5)
      unsigned	char	x4_pad1;
#endif	(X25IO_VER > 5)
};

#if	(X25IO_VER == 5)
struct addr_1 {
		char	x3_username[8 +1];	/* filled in by client */
		char	x3_calling_dte[MAXHOSTADR];
		char	x3_destination[80];	/* call destination */
      unsigned	char	x3_context;		/* If a name */
      unsigned	char	x3_network;		/* Which Network ... */
      unsigned	char	x3_cudflen;
      unsigned	char	x3_cudf[MAXCUDF];
      struct	facilities x3_facil;
};
#else	(X25IO_VER == 5)
struct addr_1 {
      struct	facilities x3_facil;
		char	x3_destination[80];	/* call destination */
		char	x3_calling_dte[MAXHOSTADR];
      unsigned	char	x3_cudf[MAXCUDF];
		char	x3_username[8 +1];	/* filled in by client */
      unsigned	char	x3_context;		/* If a name */
      unsigned	char	x3_network;		/* Which Network ... */
      unsigned	char	x3_cudflen;
      unsigned	char	x3_pad1;
      unsigned	char	x3_pad2;
      unsigned	char	x3_pad3;
};
#endif	(X25IO_VER == 5)

#define	X25B_L_STAMP	1	/* if possible, datestamp this message	   */
#define	X25B_L_DEBUG	2	/* this is a debug message (may be ignored)*/

#ifdef	FULL_X25STR
struct x25io {
  unsigned char x_version;		/* version number */
  unsigned char x_flags;
  unsigned char x_count_ms;
  unsigned char x_count_ls;
  union {
    unsigned char x2_rawbuf[1024 + 1 + TS29_BYTES];
    struct {
#if	(TS29_BYTES > 0)
      unsigned char x3_tspad;		/* pad character in case needed */
#endif
      unsigned char x3_send_type;	/* M-, D- and Q- bits */
      char x3_buf[1024]; /* We want the headers too */
    } x2_split;
    struct x2SBUF {
#if	(TS29_BYTES > 0)
      unsigned	char x3_tspad;		/* not used */
#endif
      unsigned	char x3_send_type;	/* not used */
      unsigned	char x3_ssize;     /* sanity check between client/server */
      unsigned	char x3_pad1;		/* not used */
#if	(TS29_BYTES <= 0)
      unsigned	char x3_pad2;		/* not used */
#endif
      struct addr_1 x2_sbuf;
    } x2SBUF;
  } x1_bufs;
};

#define	x_rawbuf	x1_bufs.x2_rawbuf
#define	x_buf		x1_bufs.x2_split.x3_buf
#define	x_tspad		x1_bufs.x2_split.x3_tspad
#define	x_send_type	x1_bufs.x2_split.x3_send_type

#define	x_ssize		x1_bufs.x2SBUF.x3_ssize
#define	x_sbuf		x1_bufs.x2SBUF.x2_sbuf
#define	x_facil		x_sbuf.x3_facil
#define	x_fflags	x_facil.x4_fflags
#define	x_reverse_charge x_facil.x4_reverse_charge
#define	x_recvpktsize	x_facil.x4_recvpktsize
#define	x_sendpktsize	x_facil.x4_sendpktsize
#define	x_recvwndsize	x_facil.x4_recvwndsize
#define	x_sendwndsize	x_facil.x4_sendwndsize
#define	x_recvthruput	x_facil.x4_recvthruput
#define	x_sendthruput	x_facil.x4_sendthruput
#define	x_cug_index	x_facil.x4_cug_index
#define	x_fast_select	x_facil.x4_fast_select
#define	x_rpoa		x_facil.x4_rpoa
#define	x_destination	x_sbuf.x3_destination
#define	x_calling_dte	x_sbuf.x3_calling_dte
#define	x_cudf		x_sbuf.x3_cudf
#define	x_username	x_sbuf.x3_username
#define	x_context	x_sbuf.x3_context
#define	x_network	x_sbuf.x3_network
#define	x_cudflen	x_sbuf.x3_cudflen
#define	x_pad1		x_sbuf.x3_pad1
#define	x_pad2		x_sbuf.x3_pad2
#define	x_pad3		x_sbuf.x3_pad3

#define	x25sbuf_size (sizeof (struct addr_1))
#define	x25SBUF_size (sizeof (struct x2SBUF))
struct x25io dummy;	/* Sigh .... */
#define x25hdrsize ((&dummy.x_rawbuf[0])-(unsigned char *)(&dummy.x_version))


/* x25callsetup's are initialised by the client and sent to the server */

#define PRE_CUDFLEN (sizeof(PRE_CUDF))
#define TS_CUDFLEN (sizeof(TS_CUDF))
static unsigned char PRE_CUDF[] = { 0x01, 0x5e, 0x0d, 0x00 };
static unsigned char TS_CUDF[] = { 0x7f, 0xff, 0xff, 0xff };
#endif	FULL_X25STR

#ifndef	DEF_YBTS
#define	DEF_YBTS	"TS29\0<patch area>"
#endif	DEF_YBTS
#ifndef	SERVERNAME
#define	SERVERNAME	"x25-serv\0<patch area>"
#endif	SERVERNAME
