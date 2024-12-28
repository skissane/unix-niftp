/* unix-niftp h/tab.h $Revision: 5.5 $ $Date: 90/08/01 13:33:37 $ */
/*
 *  tab.h
 *
 * last changed 8-aug-83
 * $Log:	tab.h,v $
 * Revision 5.5  90/08/01  13:33:37  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:42:18  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:26:14  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 *
 *	The structure of an entry from the queue.
 *
 *      The pointers are indexes in the text array indicating the start
 *      of the relevent string. Each string is null terminated.
 *      This file now also contains the information required from docket
 */

#define TEXTL           (200-16+512)
#define SOME            40              /* useful constants */
#ifndef	ENOUGH
#define ENOUGH          100
#endif

/* t_flags */
#define T_TYPE			0x7	/* transfer type mask */

#define T_FTP			0x1	/* file transfer */
#define T_MAIL			0x2	/* mail transfer */
#define T_JTMP			0x3	/* job transfer */
#define T_NEWS			0x4	/* news transfer */
#define T_PP			0x5	/* PP transfer */

#define REVERSE_CHARGING	0x8
#define NOTIFY_SUCCESS		0x10
#define BINARY_TRANSFER		0x20
#define WRITE_DELETE		0x40	/* delete local file after write*/

#ifdef  UCL
/* local flags for ucl */

#define OLD_PASSWD		0x80	/* we are using old password format */
#endif

#define FILE_MODES              0x100   /* set if we can't send 0x80 format */
#define	ENCODE_PW		0x200	/* do trivial encoding of password */

#define val     unsigned short

struct  tab{
	short   t_access;       /* access modes */
	short   t_flags;        /* Option flags for request */
	short   bin_size;       /* binary word size - only sent if binary */
	short   status;         /* status of transfer - local status */
	short   tptr;           /* next available slot in tab on exit */
	short   l_usr_id;       /* local user id */
	short   l_grp_id;       /* local group id */
	short   l_nretries;     /* Number retry attempts done */
	long    l_nextattmpt;   /* time of next attempt if requeued */
	long    t_queued;       /* time of queueing of job */
	short   l_network;      /* name of host control file */
	short   l_hname;        /* name of remote host */
	short   l_jtmpname;     /* name of jtmp file */
	short   l_from;         /* the sender of mail */
	short   r_usr_n;        /* remote user name */
	short   r_usr_p;        /* remote user password */
	short   r_fil_n;        /* remote file name */
	short   r_fil_p;        /* remote file password */
	short   l_fil_n;        /* local file name */
	short   account;        /* remote account name */
	short   acc_pass;       /* account password */
	short   dev_type;       /* o/p device type, only used for transmit */
	short   dev_tqual;      /* o/p device type qualifier */
	short   op_mess;        /* operator message */
	short   mon_mes;        /* monitor message built up by lftp */
	short   specopts;       /* string for special options */
	long    l_docket;        /* number of transfer docket */
	char    text[TEXTL];
	struct  docket {                         /* docket information */
		val     last_rlen;              /* Now only needs 1 file */
		val     st_of_tran;             /* to store all the info */
		long    last_count;             /* on p side, helps speed */
		val     last_mark;
		val     rec_mark;
		val     lr_reclen;
		long    lr_bcount;
		val     doc_p[256];
		short   uid,gid;
		long    transfer_id;
		char    hname[SOME];
		char    tname[ENOUGH];
		char    rname[ENOUGH];
		} udocket;
	};

extern  struct  tab     tab;

#undef  val

/*
	states of transfer --- requests
*/
#define DONESTATE       6       /* transfer complete */
#define XDONESTATE	0	/* old donestate */
#define GOSTATE         1       /* transfer in progress */
#define TRYINGSTATE     2       /* In level 0 setup */
#define PENDINGSTATE    3       /* not tried yet */
#define REQSTATE        4       /* transfer failed - try again later*/
#define RESUMESTATE     5       /* can resume later - new - possibly */
#define ABORTSTATE      -1      /* transfer aborted */
#define REJECTSTATE     -2      /* transfer rejected by Q or this side */
#define CANCELSTATE     -3      /* Cancelled by ftpstate */
#define TIMEOUTSTATE    -4      /* timedout - try again later */
#define FABORTSTATE     -5      /* aborted. failed on the STOPACK */

#define QUEUEMODE 0666          /* The access mode for the queue */
