/* unix-niftp lib/pqproc/data.c $Revision: 5.6 $ $Date: 1991/06/07 17:01:40 $ */
/*
 * data definitions for the ftp P and Q processes
 *
 * $Log: data.c,v $
 * Revision 5.6  1991/06/07  17:01:40  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:36:43  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:49:45  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:24:37  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:12:24  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:32:19  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:07:49  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:00:26  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:49:05  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include "ftp.h"
#include "files.h"

jmp_buf rcall;

#ifdef  MAIL
int     mailer;
#ifdef	PP
int	pp_spooled;
#endif	PP
#endif

struct  tab     tab;            /* the queue entry */
struct  passwd  *cur_user;
int     direction;              /* the current transfer direction */
int     tstate;                 /* current state of ftp */
char    ltranstype;
long    bcount;                 /* count of number of chars read */
val     reclen;                 /* current length of record */
long    last_count;             /* last char count recieved */
val     last_rlen;              /* last record length recieved */
val     tsc_attr;
val     datatype;               /* data transfer type */
int     wordcount;              /* count of number of chars arrived */
int     bin_tran;
val     bin_format;
val     bin_size;               /* length of word in binary transfer */
val     st_of_tran;             /* state of transfer */
val     format;                 /* text formatting */
val     f_access;               /* access permissions */
val     max_rec_siz;            /* maximum record size */
val     acknowind;              /* acknowledgement window */
val     facilities;             /* facilities */
int     time_out;               /* current timeout period */
long    transfer_id;            /* transfer id for current transfer */
int     code;                   /* current transfer code text/binary */
val     rec_mark;               /* last mark received */
val     lastmark;               /* last mark sent */
val     lr_reclen;              /* last record length recieved */
long    lr_bcount;              /* last marks bcount */
int     ok;                     /* viability of transfer */
int     try_resuming;           /* try resuming */
int     may_resume;             /* can resume */
int     rej_resume;             /* rejected the resumption */
val     old_status;
int     nrestarts;              /* number of restarts allowed when out of
				 * disk space */
int     nproterrs;              /* number of protocol errors we will allow
				 * before we snuff the connection */
int     spopts_set;             /* special options have been set */
int	q_daemon;		/* Q process running under a daemon.
				   -> no arg and don't re-open */
uidtype uid,gid;                /* uid and group id of user */
short   ecode;                  /* error code number */
char    *hostname;              /* host name */
char    *localname;             /* my file name that the ftp uses */
char    *realname;              /* the actual file name */
char    *argstring;
char    *sftstrings[STRINGCOUNT];       /* the string attributes */
extern  struct  sftparams sfts[];

#ifdef JTMP
/* jtmp variables */

char    *jtmpfile;              /* name of the jtmp file (if any ) */
int     jtmp_fd = -1;           /* fd of jtmp file ( if any ) */
#endif JTMP

/* docket variables */

char    docketname[ENOUGH];      /* name of docket */
int     docket_fd = -1;          /* file descriptor of docket */
val     t_st_of_tran;           /* local state of transfer - useful ? */
uidtype t_uid,t_gid;

short   error;                  /* current error number ( rewrite queue ) */
char    told_to_die;
long    time_read;
int     qfd = -1;               /* queue file descriptor */

node	net_io;			/* the net node itself */
int     net_wcount;             /* the number of characters written */
int     net_rcount;             /* the number of characters read */
int     net_fail();
int     (*net_error)() = net_fail;      /* the error trap routine */
int	open(), read(), write(), close();
int	(*openp)()	= open;
int	(*readp)()	= read;
int	(*writep)()	= write;
int	(*closep)()	= close;
char    net_open;       /* set if connection is open */

unsigned char    *recp; /* pointer to current position in sub_record */
unsigned char    *recst;/* pointer to start of sub_record buffer */
char	*altpwfile;	/* Alternative Password File */
char	*guest_name;	/* Name of "guest" accounts */
int     sreclen;        /* current length of buffer used ( bytes ) */
int     max_b_siz;      /* maximum no bytes/word buffer allocated */
int     wdbcount;       /* number of bytes transfered - current word */
int     wdcount;        /* number of words transfered - current sub record */
int     wrd_bin_siz;    /* size of word (usually 1 ) */

unsigned char    *rrecst;        /* receive side buffer pointer */
unsigned char    *rrecptr;       /* pointer into buffer */
int     rsreclen;       /* number of bytes in buffer */

int	trans_data0;	/* the raw data in the 0th word */
int	trans_wind;	/* Window size as set by ftptrans */
int	trans_pkts;	/* Packet size as set by ftptrans */
int	trans_call;	/* Call types as set by ftptrans */
int	trans_revc;	/* Should the call be reverse charged */
int	trans_unixniftp;/* Is the remote end a unix-niftp ? */
int	trans_cug;	/* Closed User group */
int	trans_fcs;	/* <0 -> Use Non Fast Call Select, > 0 -> use FCS */
int	sig_bits	= 10;	/* special signal bits for debugging */

int     fnleft;                 /* for block io */
int     f_fd = -1;
char    r_flag;                 /* set on every read into buffer */
unsigned char    io_buff[BLOCKSIZ];      /* the io buffer */

int     ftp_print;              /* for diagnostic printing */
char	why_unknown_host[ENOUGH];

char	escape_double[]	= "\"$`\\";

/* table of error messages explaining what went wrong*/

char    *ermsg[] = {
	"Unimplemented attribute (never seen)",
	"Bad format attribute",
	"Bad binary word format",
	"Unimplemented restart mark",
	"Unimplemented binary word size",
	"Can't resume transfer",
	"Unimplemented access mode",
	"Not Niftp-80 protocol",
	"Cannot access file",
	"Password required",
	"Unknown user name",
	"File not printed",
	"File not deleted",
	"Docket not found",
	"Passwords mismatch",
	"Unknown protocol attribute",
	"Unimplemented attribute value",
	"Bad attribute value",
	"Receive file deleted",
	"Invalid attribute negotiation",
	"Attribute type is wrong",
	"Attribute type is wrong (RPOS not EQ ANY)",
	"Unimplemeneted attribute value",
	"Disk nearly full",
	"File not processed",
	"Unregistered calling address - ANON access only",
	"Guest account only allows READ ONLY access",
	"Filename given when not expected",
	"Username given when not expected",
	"Username and Filename given when not expected",
	"Unknown conxtext (e.g. not ftp, mail, jtmp or news)",
	"No password give for guest access (should be an email address)",
	"No '@' in email address in guest access password",
	"Banned user file unreadable",
	"User not allowed to transfer files",
	"PP rejected call at start (misconfigured?)",
	"Mailer rejected call during transfer",
	"PP rejected call at close (invalid recipient?)",
	"Connection to destination failed"
	};
