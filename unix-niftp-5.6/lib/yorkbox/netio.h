/*
 * rcsid[] = "$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/lib/yorkbox/netio.h,v 5.5 90/08/01 13:39:38 pb Exp $";
 *	YORK UNIX-X25 COMMUNICATIONS SOFTWARE
 *
 *	Header file for netio.c
 *
 *	Copyright (c) 1986 The University of York, the Science and
 *	Engineering Research Council, and University College London.
 *
 *	$State: Exp $
 *	$Log:	netio.h,v $
 * Revision 5.5  90/08/01  13:39:38  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:57:14  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:58:49  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 * Revision 22.1  86/08/29  08:12:04  pb
 * The D2.2 copy ....
 * 
 * Revision 2.2  86/06/01  12:37:03  pb
 * /tmp/1
 * 
 *
 *	Revision 22.1	Release 2.2
 *
 */

#ifndef	HHALL_GOT
#include "hhall.h"
#endif	HHALL_GOT
#ifndef	_NODEF
#ifndef	NCALLS
#define	NCALLS	1
#endif	NCALLS
int     _NTS = NCALLS;
#else
extern int  _NTS;
#endif	_NODEF

/*
 *			tsp->t_flags (1)
 *					
 */
#define	REV_CHARGE	01
#define	FAST_SELECT	0200

/*
 *			tsp->t_stat
 *				Set by CALL_STATS in tsevent to F_val.
 */
#if !(defined (HHP) || defined (HHQ))
#define	ST_NGEN		01	/* ? Network generated ?? */
#define	ST_TGEN		02	/* Locally generated stats (e.g. tstats())
				   */
#define	ST_CCNF		0200	/* ? Close No Fn -> no clr/diag ?? */
#endif	!HHP && !HHQ

#define	DTELEN		20	/* Used below only	 */
#define	PKTSIZ		128	/* Used by twrite.c	 */
#define	MAXSIZE		255	/* Maximum size of a rebuilt packet */
#define	CSTATSIZE	30	/* ???? sizeof(STATISTICS) [T_sstat] ???? 
				*/
#define	SEGSIZE		64	/* Size of a segment !	 */
#define	FRAGLEN		(SEGSIZE -1)


/*
 * Message type byte values.
 *			FEP_msg->F_type
 */

#define	NETDOWN		10	/* The net is down	 */
#define	L2TRACE		12	/* Tell me about level 2 */
#define	CONGESTED	13	/* Temporary congestion in the fep - retry */

/*
* These are given here out of order to allow HHP and HHQ and PAD to
* have lots of defines.
*/
#define	_CLOSE		18	/* X25 DISCONNECT (3)	 */
#define	_RESET		19	/* X25 RESET (3)	 */
#define	_EXPEDITE	22	/* X25 EXPEDITE	(1)	 */
#if !(defined (HHP) || defined (HHQ))
#define	_OPEN		16	/* X25 CONNECT (4)	 */
#define	_ACCEPT		17	/* X25 ACCEPT (3)	 */
#define	_ADDRESS	20	/* X25 ADDRESS (2)	 */
#define	_LISTEN		25	/* YB  listen for req	 */
#define	_TSINIT		26	/* YB  start up channel	 */
#define	_SENDDATA	27	/* YB  write -> network	 */
#define	_FETCHDATA	28	/* YB  read  <- network	 */
#define	FECCLOSE	29	/* YB  ??		 */
#define	CALL_ABORT	30	/* x25 call aborted	 */
#define	AOK		31	/* YB  ok response	 */
#define	_RST_ACK	32	/* Ack RESET->other end	 */
#define	CALL_STATS	0140	/* _CLOSE + stats info	 */
#endif	!HHP || !HHQ
/*
*			tsp->t_flags (2)
*/
#define	TIDLE		0
#define	TOPEN		01	/* Channel open	 */
#define	TABORTED	02	/* ? */
#define	TDATAREQ	04	/* User has just asked for data -> buffer 
				*/
#define	TDATAHERE	010	/* We have unsolicited data */
#define	TCALLSTATS	020	/* Stats for this call already arrived
				   (CLOSE) */
#define	TCALLING	040	/* ? */
#define	TREVCHARGE	0100	/* ? */
#define	TINUSE		0200	/* ? */
#define	TWAITACCEPT	0400	/* ? */
#define	TNS		01000	/* ? */
#define	TTRYING		02000	/* ? -> Dont account */

#if !(defined(HHP) || defined (HHQ))
/*
 * structure used to read a FEP message
 */
typedef char    FEP_msg;
#define def_buf(name)	FEP_msg	name[MAXSIZE]
#define fep_type(msg)	(msg[0])
#define fepp_type(msg)	(msg[0])
#define fep_val(msg)	(msg[1])
#define fepp_val(msg)	(msg[1])
#define fep_data(msg)	(&msg[2])
#define fepp_data(msg)	(&(msg[2]))
#endif	!HHP && !HHQ
typedef struct tsbuf {
    struct tsbuf   *b_next;
    char    b_push;
    int     b_off;
    int     b_len;
    char    b_data[1];
}                       TBUF;

#define	TSHANDLER 	0
#define	X25HANDLER	2

#define	C_WAITING	01
#define	IN_PROGRESS	02
#define	TERMINATED	04
#define	INCOMING	010
#define	OUTGOING	020
#define	ABORTED		040
#define	NOSTATS		0100
#define	REFUSED		0200


struct u_ac {
    char    a_user[24];
    long    a_budget;
    long    a_accum;
};


typedef struct stats {
    char    c_flag;
    char    c_clr;
    char    c_diag;
    char    p_days;
    char    p_hours;
    char    p_minutes;
    char    p_seconds;
    long    pr_seg;
    long    pt_seg;
}                       STATISTICS;

extern char *NETPATH;

typedef struct tsp {
    short   chan;		/* channel num is xtmp file rec key */
    long    statso;		/* offset into stats file for stats rec */
    char   *DTE;		/* DTE address (t_dteadr)	 */
    char   *ADR;		/* set in OPEN if TNS		 */
    char   *TEXT;		/* ....				 */
    char    ERR;		/* the error reason (CLOSE)	 */
    char    TYPE;		/* fep_type(tsp)		 */
    int     t_fid;		/* file descriptor of access path */
    short   t_flags;		/* Reverse charge ? ... STATE */
    int     t_pid;		/* PID of process */
    int     t_uid;		/* this user's identifier */
    char    t_exp;		/* received expedited byte */
    int     t_read;		/* actual length of data read */
    int     t_write;		/* write offset */
    int     t_rstat;		/* return status value */
    long    t_start;		/* time of start of call */
    TBUF    *t_bq;		/* queue of received data buffers */
    char    t_pno;		/* parameter currently being handled */
    char    t_frag;		/* fragment length */
    char    t_dteadr[DTELEN];	/* calling dte address */
    char    t_param[MAXSIZE];	/* received parameter */
    char    t_text[MAXSIZE];	/* received text */
    union {
	char       T_cstat[CSTATSIZE];/* received call statistics */
        STATISTICS T_stat;
    }  t_un;
    char   *t_adrsav;		/* saved DISC address param */
    char   *t_txtsav;		/* saved DISC text param */
    char    t_errsav;		/* saved DISC err param */
    char    t_docharge;		/* must charge for the call */
    long    t_charge;		/* cost of this call */
} TS;
#if !(defined (HHP) || defined (HHQ))
#define	t_cstat	t_un.T_cstat
#define	t_stat	t_un.T_stat
#endif	!HHP && !HHQ

#ifndef	_NODEF
TS tsp[NCALLS];
char    xstate[NCALLS];
#else
extern  TS tsp[];
extern char xstate[];
#endif

extern  TS * TS_CALLED;
extern char terrno;
extern  STATISTICS * TS_STAT;

#define	tadr(p)		(p)->ADR
#define	ttext(p)	(p)->TEXT
#define	tmess(p)	(p)->TYPE
#define	terrp(p)	(p)->ERR
#define	tstat()		TS_STAT
#define	tdata(p)	((p)->t_flags & TDATAHERE)
#define	tdte(p)		(p)->DTE
#define	tcalled(p)	(p)->ADR
#define	tcalling(p)	(p)->TEXT
#define	tcharge(p)	(p)->t_charge
#define texp(p)		(p)->t_exp
#if !(defined (HHP) || defined (HHQ))
#ifdef	BSD4_2
#define	FIOPINTE	_IO(x, 0)
#define	FIOIABRT	_IO(x, 1)
#define	FIOPCCNT	_IO(x, 2)
#define	FIOPINTQ	_IO(x, 3)
#define	FIOIDSBL	_IO(x, 4)
#define	FIOICOLC	_IOR(x, 5, int)
#else
#define	FIOPINTE	0
#define	FIOIABRT	1
#define	FIOPCCNT	2
#define	FIOPINTQ	3
#define	FIOIDSBL	4
#define	FIOICOLC	5
#endif	BSD4_2
#endif	!HHP && !HHQ

extern  STATISTICS * tclose ();
