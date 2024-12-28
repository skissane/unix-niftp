/* unix-niftp h/nrs.h
 * rcsid[]="$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/h/stat.h,v 5.5 90/08/01 13:33:34 pb Exp $"
 *
 * $Log:	stat.h,v $
 * Revision 5.5  90/08/01  13:33:34  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:13:39  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.3  87/12/09  16:57:36  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/12/09  16:51:38  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0  87/03/23  03:23:37  bin
 * 
 */

#define	MAXNAMEL	20
#define	MAXADDRL	100

#define	S_DEAD		-1	/* Shoule be unlinked */
#define	S_LISTEN	1	/* Listening */
#define	S_OPENING	2	/* Opening outgoing call */
#define	S_DECODING	3	/* DBM lookup .. */
#define	S_IDLE		4	/* Just started ... */
#define	S_ERRCLOSE	5	/* Closing after error */
#define	S_LISTENFAILED	6	/* Listen failed */
#define	S_SFT		7	/* awaiting SFT */
#define	S_DATA		8	/* done SFT, now data */
#define	S_DONE		9	/* Transfer over, close down ... */
#define	S_AWAITSTOP	10	/* awaiting the STOP */
#define	S_PROCESS	11	/* deliver news/mail/jtmp/etc */
#define	S_PROCESSED	12	/* Done processing */
#define	S_OPENFAILED	13	/* the open failed */
#define	S_FINDP		14	/* looking for work */
#define	S_FOUNDP	15	/* found work -- now set up call */
#define	S_RELISTEN	16	/* relistening after timeout */
#define	S_FAILISTEN	17	/* listen failed, sleep & retry. */
#define	S_	1	/*  */
#define	S_	1	/*  */

#define	L_SET		0 /* lseek arg */

struct s_stat {
	long	s_pid;			/* Process id */
	long	s_time;			/* time this action started */
	long	s_val;			/* State specific info */
	int	s_stat;			/* Current state */
	char	s_addr[MAXADDRL];	/* remote host address */
	char	s_name[MAXADDRL];	/* remote host name (or [dom lit]) */
	char	s_serv[MAXNAMEL];	/* service name */
	char	s_newl;			/* newline at end to look nice .. */
};
