/* unix-niftp h/ftp.h $Revision: 5.6 $ $Date: 1991/06/07 17:00:04 $ */
/*
 * ftp.h - all constant definitions - information. structures etc.
 * last changed 31-may-83
 *
 * $Log: ftp.h,v $
 * Revision 5.6  1991/06/07  17:00:04  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:33:05  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:40:27  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:12:40  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:09:18  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:26:40  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:06:47  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  11:59:14  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:25:39  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */

#include "opts.h"

#include        <signal.h>
#include        <setjmp.h>
#include        <stdio.h>
/*
 * basic - termination initialisation commands
 */

#define STOP            0x00
#define GO              0x01
#define RPOS            0x02
#define RNEG            0x03
#define SFT             0x04
#define STOPACK         0x05

/*
 * second level - transfer control commands
 */

#define SS              0x40
#define MS              0x41
#define CS              0x42
#define ES              0x43
#define RR              0x44
#define MR              0x45
#define QR              0x46
#define ER              0x47

/* Parameter names - in numeric order - easy to lookup in book.... */

#define PROTOCOL        0x00            /* protocol id */
#define ACCESS          0x01            /* access mode */
#define TEXTCODE        0x02            /* text transfer code */
#define TEXTFORM        0x03            /* text formatting */
#define BINFORM         0x04            /* binary format */
#define MTRS            0x05            /* maximum transfer record size */
#define TRANSLIM        0x06            /* transmission limit */
#define DATAEST         0x07            /* data estimate */
#define TRANSID         0x08            /* transfer id */
#define PTCN            0x09            /* private transfer code name */
#define ACKWIND         0x0A            /* acknowledgement window */
#define INITRESM        0x0B            /* initial restart mark */
#define MINTIME         0x0D            /* minimum timeout */
#define FACIL           0x0E            /* facilities */
#define STOFTRAN        0x0F            /* state of transfer */
#define DATATYPE        0x20            /* data type */
#define DELIMPRE        0x21            /* delimiter preservation */
#define TEXTSTC         0x22            /* text storage code */
#define HORIZTAB        0x23            /* horizontal tabs */
#define BINWORD         0x24            /* binary word size */
#define MSTRECS         0x25            /* maximum storage record size */
#define PAGEWID         0x26            /* page width */
#define PAGELEN         0x27            /* page length */
#define PRISTCD         0x29            /* private storage code */
#define FILENAME        0x40            /* file name */
#define USERNAME        0x42            /* user name */
#define USERPWD         0x44            /* user password */
#define FILEPWD         0x45            /* file password */
#define ACCOUNT         0x4A            /* account name */
#define ACCNTPWD        0x4B            /* account password */
#define OUTDEVT         0x50            /* output device type */
#define DEVTQ           0x51            /* device type qualifier */
#define FILESIZE        0x60            /* file size */
#define ACTMESS         0x70            /* action message */
#define INFMESS         0x71            /* information message */
#define SPECOPT         0x80            /* special options */

/* qualifier constants - no easy memory aid */

#define MONFLAG         0x80            /* monitor */
#define FORMAT          0x30            /* format mask */
#define ATTRIBUNKNOWN   0x00            /* attribute unknown */
#define NOVALUE         0x10            /* novalue given */
#define INTEGER         0x20            /* integer value */
#define STRING          0x30            /* string value */

#define OPER            0x07            /* operator mask */

#define EQ              0x02            /* equal */
#define LE              0x03            /* less than or equal */
#define NE              0x05            /* not equal */
#define GE              0x06            /* greater than or equal */
#define ANY             0x07            /* any */

/* Access modes */
#define	ACC_MO	0x0001		/* MAKE ONLY */
#define	ACC_RO	0x0002		/* REPLACE ONLY */
#define	ACC_AO	0x0004		/* APPEND ONLY */
#define	ACC_RES	0x0100		/* RESUMING */
#define	ACC_ROM	(ACC_MO|ACC_RO)	/* REPLACE OR MAKE */
#define	ACC_AOM	(ACC_MO|ACC_AO)	/* APPEND OR MAKE */
#define ACC_TJI	0x2001          /* TAKE JOB INPUT */
#define ACC_TJO	0x4001          /* TAKE JOB OUTPUT */
#define	ACC_GET	0x8000		/* Some sort of get */
#define	ACC_RAR	0x8001		/* READ AND REMOVE */
#define	ACC_RDO	0x8002		/* READ ONLY */
#define	ACC_DR	0x8004		/* DESTRUCTIVE READ */
#define	ACC_GJI	0xA001		/* GIVE JOB INPUT */
#define	ACC_GJO	0xC001		/* GIVE JOB OUTPUT */

/* definition of various string pointer locations in sftstrings */

#define pritcn   (sftstrings[0])        /* private transfer code name */
#define horiztab (sftstrings[1])        /* delimiter preservation */
#define prstcd   (sftstrings[2])        /* private storage code name */
#define filename (sftstrings[3])        /* filename */
#define username (sftstrings[4])        /* username */
#define usrpaswd (sftstrings[5])        /* username password */
#define filpaswd (sftstrings[6])        /* filename password */
#define accounts (sftstrings[7])        /* account */
#define accpaswd (sftstrings[8])        /* account password */
#define outdev   (sftstrings[9])        /* output device type */
#define devtypq  (sftstrings[10])       /* device type qualifier */
#define actnmsg  (sftstrings[11])       /* action message */
#define infomsg  (sftstrings[12])       /* information message */
#define specopt  (sftstrings[14])       /* special option string */

typedef unsigned short  val;    /* define a common type */

typedef short   uidtype;        /* the size of a user id number */

struct  sftparams {
	char    attribute;      /* attribute */
	unsigned char    qualifier;      /* qualifier */
	char    sflags;         /* local flags for ftp use */
	unsigned char    squalifier;     /* qualifier that is to be sent */
	val     ivalue;         /* value - given as a parameter */
	val     ovalue;         /* our default value for parameter */
	val     dvalue;         /* default value - for all things */
	};

/* flags for sftparams flag word - see sfttab.h*/

#define USERSET         0x1             /* user settable */
#define FROMSEL         0x2             /* from selector */
#define MUSTSEND        0x4             /* must send */
#define TOSEND          0x8             /* to send */
#define NEVERSEND       0x10            /* never send */
#define FAILURE         0x20            /* failure */
#define NOSUPPORT       0x40            /* no support */
#define BIT             0x80            /* as opposed to unsigned */

/* more ftp definitions */

#define TCC_COMMAND     0x200           /* had a level 2 command */
#define END_REC_BIT     0x400           /* got to end of record */
#define MASK            0xFF            /* mask to get chars into ints */
#define INFINITY        0xFFFF          /* lots + lots */
#define STRINGCOUNT     15              /* size of string array */
#define Q_SLEEP         1200            /* secs sleep time for processing Q*/
#define	MSGSEPCH	'\n'		/* internal separator in msgs */
#define	MSGSEPSTR	"\n"		/* MUST KEEP IN STEP WITH ABOVE */

#define NFILES          20

#ifndef	CONF_MAX
#define CONF_MAX        50      /* maximum configurable values */
#endif	/* CONF_MAX */

#ifdef  BUFSIZ
#define BLOCKSIZ        BUFSIZ
#else
#define BLOCKSIZ        512     /* different on larger machines */
#endif

#define SOME            40              /* useful constants */
#define ENOUGH          100
#define TIMEOUT         0x250
#define READSIZE        10000           /* should be as large as possible */

/*
 * seeing as we send marks at 2k intervals these are the constants for that
 * MARK_MASK is the mask to say that we are at a 2k block.
 * LSHIFT is log(2k) for shifting the mark number to convert back to the
 * place in the file.
 */

#define MARK_MASK       0x07FF
#define LSHIFT          11

#define MAXSUBRECSIZ    63              /* maximum size of sub record */
#define END_REC         0x80            /* end of record bit */
#define COMPRESS_BIT    0x40            /* compression bit */

/* facilities bits */

#define TCOMPRESS       0x1             /* text compression */
#define LRESUME         0x2             /* later resumptions */
#define RESTARTS        0x4             /* restarts */
#define MUSTACK         0x8             /* mark acknowlegement must occur */
#define HOLDS           0x10            /* holds */
#define BCOMPRESS       0x40            /* binary compression */

/* direction bits */

#define TRANSMIT        01
#define RECEIVE         02

/* various state bits - since the ftp is a finite state machine */

#define STOPs   0x30
#define GOs     0x31
#define RPOSs   0x32
#define RNEGs   0x33
#define SFTs    0x34
#define STOPACKs 0x36

#define WAIT    0x22
#define OKs     0x23
#define FAIL    0x24

#define DATA    0x25
#define RRs     0x26
#define HOLD    0x27
#define HORR    0x28
#define MRs     0x29
#define ESok    0x2A
#define ESe     0x2B

#define PEND    0x2C
#define PERR    0x2D
#define QRok    0x2E
#define QRe     0x2F

/* state of transfer states - base values - others will appear */

#define VIABLE          0x0000

#define REJECTED        0x1000
#define REJECTED_MSG    0x0001
#define REJECTED_ATTRIB 0x0002
#define REJECTED_POSS   0x0003
#define REJECTED_RESUME 0x0004

#define TERMINATED      0x2000
#define TERMINATED_OK   0x0000
#define TERMINATED_MSG  0x0001

#define ABORTED         0x3010
#define ABORTED_IMPOSS  0x0010
#define ABORTED_POSS    0x0011

/* some character definitions */

#define NL              '\n'
#define CR              '\r'
#define NP              '\f'    /* I think */
#define LF              '\n'
#define SP              ' '

/* Qualified commands */
#define	YB_CONNECT	0x10
#define	YB_ACCEPT	0x11
#define	YB_DISCONNECT	0x12
#define	YB_RESET	0x13
#define	YB_DATA		0x15
#define	YB_ADDRESS	0x14
#define	YB_EXPEDITED	0x16

/* mask bits to say which qualified command are allowed */
#define	AQ_CONNECT	(1 << YB_CONNECT)
#define	AQ_ACCEPT	(1 << YB_ACCEPT)
#define	AQ_DISCONNECT	(1 << YB_DISCONNECT)
#define	AQ_RESET	(1 << YB_RESET)
#define	AQ_DATA		(1 << YB_DATA)
#define	AQ_ADDRESS	(1 << YB_ADDRESS)
#define	AQ_EXPEDITED	(1 << YB_EXPEDITED)

/* include some other definitions */

#include "tab.h"

#include        <pwd.h>         /* password information */
#include        <sys/stat.h>

#define docp    tab.udocket.doc_p

typedef struct  docket   *docketp;

struct  sftparams *sftp();
char    *malloc();
char    *strcpy();
char    *utoa();
char    *ltoa();
char    *getenv();
long    bsize();

/* defines for the mail interface - At least for out system */

extern  errno;                  /* for error numbers */

/* define some global variables */

struct  net_io {	       /* public interface to the network IO librarys */
	int     read_count;
	int     write_count;
	unsigned char	*read_buffer;
	unsigned char	*write_buffer;
	} ;
typedef struct  net_io node;
typedef node    *nodep;

/*
 * several macros to deal with various possible transfer types
 */

#define	ISFTP(x)	((x) == T_FTP)
#define	ISMAIL(x)	((x) == T_MAIL)
#define	ISJTMP(x)	((x) == T_JTMP)
#define	ISNEWS(x)	((x) == T_NEWS)
#define	ISPP(x)		((x) == T_PP)
#define	ISMAILS(x)	(ISMAIL(x) || ISPP(x))

#define FTPTRANS	(ltranstype == T_FTP)
/* due to management decisions we can't just reject this */
#define MAILTRANS	(ltranstype == T_MAIL)
#define JTMPTRANS	(ltranstype == T_JTMP)
#define NEWSTRANS	(ltranstype == T_NEWS)
#define PPTRANS		(ltranstype == T_PP)
#define MAILSTRANS	(MAILTRANS || PPTRANS)

struct addrtype {
	char	*at_str;
	int	at_type;
};

extern struct addrtype p_addrtype[];
extern struct addrtype q_addrtype[];

/* For local notification */
#define	ADD_TO		1
#define	ADD_SUBJ	2
#define	ADD_BLANK	4
/* For delivery from remote site */
#define	ADD_RECV	8	/* Add a Received: from line */
#define	CATCH_ALL	16	/* Catch ANY error ! */
#define	SEND_ASIS	32	/* Do not strip of the recipient list */
/* for returning bounced mail */
#define	ESCAPE_USER	64	/* ecaspe user as for double quotes */
#define	ESCAPE_SUBJECT	128	/* ecaspe user as for double quotes */

struct mailfmt{
	char	*prog;
	char	*form;		/* local notification */
	char	*deliver;	/* delivery of FTP.MAIL data */
	int	flags;
};
extern struct mailfmt mailfmt[];

extern char *ourname;
extern char *outdtype[];

extern  jmp_buf rcall;

#ifdef  MAIL
extern  int     mailer;
#endif

extern  char	version[];
extern  struct  passwd  *cur_user;
extern  int     direction;
extern  int     tstate;
extern  long    bcount;
extern  val     reclen;
extern  long    last_count;
extern  val     last_rlen;
extern  val     tsc_attr;
extern  val     datatype;
extern  int     wordcount;
extern  int     bin_tran;
extern  val     bin_format;
extern  val     bin_size;

extern  val     st_of_tran;
extern  val     format;
extern  val     f_access;
extern  val     max_rec_siz;
extern  val     acknowind;
extern  val     facilities;
extern  int     time_out;
extern  long    transfer_id;
extern  int     code;
extern  val     rec_mark;
extern  val     lastmark;
extern  val     lr_reclen;
extern  long    lr_bcount;
extern  int     ok;
extern  int     try_resuming;
extern  int     may_resume;
extern  int     rej_resume;
extern  val     old_status;
extern  int     nrestarts;
extern  int     nproterrs;
extern  int     spopts_set;
extern	int	q_daemon;

extern  uidtype uid,gid;
extern  short   ecode;
extern  char    *hostname;
extern  char    *localname;
extern  char    *realname;
extern  char    *argstring;
extern  char    *jtmpfile;

extern  char    *sftstrings[];
extern  struct  sftparams sfts[];

extern  char    docketname[];
extern  int     docket_fd;
extern  val     t_st_of_tran;
extern  uidtype t_uid,t_gid;
extern  char    qdoced[];
extern  short   error;
extern  char    told_to_die;
extern  long    time_read;
extern  int     qfd;
extern  int     jtmp_fd;

extern  node	net_io;
extern  int     net_wcount;
extern  int     net_rcount;
extern  int     (*net_error)();
extern	int	(*openp)();
extern	int	(*readp)();
extern	int	(*writep)();
extern	int	(*closep)();
extern  char    net_open;
extern  unsigned char	net_read_buffer[];
extern  unsigned char	net_write_buffer[];
extern  char	t_addr[];
extern  unsigned char    *recp;
extern  unsigned char    *recst;
extern  int     sreclen;
extern  int     max_b_siz;
extern  int     wdbcount;
extern  int     wdcount;
extern  int     wrd_bin_siz;

extern  unsigned char    *rrecst;
extern  unsigned char    *rrecptr;
extern  int     rsreclen;

extern	int	trans_data0;
extern	int	trans_wind;
extern	int	trans_pkts;
extern	int	trans_call;
extern	int	trans_revc;
extern	int	trans_unixniftp;
extern	int	trans_cug;
extern	int	trans_fcs;

extern  int     fnleft;
extern  int     f_fd;
extern  char    r_flag;
extern  unsigned char    io_buff[];
extern  int     ftp_print;
extern  char	*ermsg[];
extern	long	allow_qmask;		/* Allowed Qualified commands */
extern	int	read_x25_bits;		/* QMD bits from last X25 read */

/*#define Unimplemented attribute	1*/
#define	ER_BAD_FORMAT_ATTRIB		2
#define	ER_WORD_FORMAT			3
#define	ER_UNIMPL_RESTART_MARK		4
#define	ER_UNIMPL_BIN_WORD_SIZE		5
#define	ER_CANT_RESUME			6
#define	ER_UNIMPL_ACCESS		7
#define	ER_NOT_NIFTP80			8
#define	ER_FILE_ACCESS			9
#define	ER_NEED_PW			10
#define	ER_UNKNOWN_USER			11
#define	ER_NOT_PRINTED			12
#define	ER_NOT_DELETED			13
#define	ER_NO_DOCKET			14
#define	ER_BAD_PW			15
#define	ER_UNKNOWN_ATTRIB		16
#define	ER_UNIMPL_ATTRIB		17
#define	ER_BAD_ATTRIB_VAL		18
#define	ER_RCVFILE_DELETED		19
#define	ER_INVALID_ATTRIB_NEGOTIATION	20
#define	ER_WRONG_ATTRIB_TYPE		21
#define	ER_WRONG_ATTRIB_TYPE_NOT_EQ_ANY	22
#define	ER_UNIMPL_ATTRIB_VAL		23
#define	ER_DISK_FULL			24
#define	ER_NOT_PROCESSED		25
#define	ER_BAD_CALLING			26
#define	ER_GUEST_ACCESS			27
#define	ER_FILENAME_GIVEN		28
#define	ER_USERNAME_GIVEN		29
#define	ER_USERFILE_GIVEN		30
#define	ER_UNKNOWN_CONTEXT		31
#define	ER_NO_PASSWD			32
#define	ER_NO_AT_IN_PW			33
#define	ER_NO_BANNEDFILE		34
#define	ER_BANNED_USER			35
#define	ER_MAIL_START			36
#define	ER_MAIL_DATA			37
#define	ER_MAIL_END			38
