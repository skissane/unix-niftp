/* unix-niftp h/nrs.h $Revision: 5.6.1.6 $ $Date: 1993/01/10 07:09:10 $ */
/*
 * $Log: nrs.h,v $
 * Revision 5.6.1.6  1993/01/10  07:09:10  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1991/06/07  17:00:12  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:33:18  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.3  89/07/16  12:02:40  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  14:41:23  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:13:25  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.4  88/01/28  06:09:35  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:27:15  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  13:26:02  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:03:30  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.1  87/09/09  15:52:13  pb
 * add [LQ]statfile & KILLSPOOL
 * 
 * Revision 5.0  87/03/23  03:23:37  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#include <stdio.h>

/*
 * the NRS defined contexts and nets
 */

#define XCONTEXT        1       /* NRS contexts:- X29 context */
#define TCONTEXT        2       /* ts29 context */
#define FCONTEXT        3       /* ftp context */
#define MCONTEXT        4       /* mail context */
#define JCONTEXT        7       /* jtmp context */
#define ICONTEXT        12      /* Information context */
#define NCONTEXT	127	/* Non NRS context:- news context */
#define	GCONTEXT	126	/* Non NRS context:- this is a gateway */

#define NET_PSS         1       /* NRS network numbers - PSS */
#define NET_JANET       2       /* Janet */
#define NET_TELEX       3       /* Telex network ?? */

#define MAXNETS		5	/* maximum number of nets */
#define JANETNET	0	/* offset of Janet info into array */
#define PSSNET		1	/* offset of PSS info. E.g. n_addrs[PSSNET] */

				/* How many transfers per TS connection */
#define	TRANS_MAX_MASK		0x0000003f
#define	TRANS_MAX_SHIFT		0
#define	TRANS_MAX_VAL(x)	((x & TRANS_MAX_MASK) >> TRANS_MAX_SHIFT)
				/* Window size				*/
#define	TRANS_WIND_MASK		0x000001c0
#define	TRANS_WIND_SHIFT	6
#define	TRANS_WIND_VAL(x)	((x & TRANS_WIND_MASK) >> TRANS_WIND_SHIFT)
				/* PKT size: 1=16, 2=32, ..		*/
#define	TRANS_PKTS_MASK		0x00000e00
#define	TRANS_PKTS_SHIFT	9
#define	TRANS_PKTS_VAL(x)	((x & TRANS_PKTS_MASK) >> TRANS_PKTS_SHIFT)
				/* How should this host be called ?	*/
#define	TRANS_CALL_MASK		0x00003000
#define	TRANS_CALL_GW1		1		/* Through Gateway 1	*/
#define	TRANS_CALL_GW2		2		/* Through Gateway 2	*/
#define	TRANS_CALL_NEVER	3		/* Never call		*/
#define	TRANS_CALL_GW1_S	(TRANS_CALL_GW1   << TRANS_CALL_SHIFT)
#define	TRANS_CALL_GW2_S	(TRANS_CALL_GW2   << TRANS_CALL_SHIFT)
#define	TRANS_CALL_NEVER_S	(TRANS_CALL_NEVER << TRANS_CALL_SHIFT)
#define	TRANS_CALL_SHIFT	12
#define	TRANS_CALL_VAL(x)	((x & TRANS_CALL_MASK) >> TRANS_CALL_SHIFT)

#define	TRANS_REVC_MASK		0x00004000
#define	TRANS_REVC_SHIFT	14
#define	TRANS_REVC_VAL(x)	((x & TRANS_REVC_MASK) >> TRANS_REVC_SHIFT)
	
#define	TRANS_UNIXNIFTP_MASK	0x00008000
#define	TRANS_UNIXNIFTP_SHIFT	15
#define	TRANS_UNIXNIFTP_VAL(x)	((x & TRANS_UNIXNIFTP_MASK) >> TRANS_UNIXNIFTP_SHIFT)

#define	TRANS_FCS_MASK		0x00010000
#define	TRANS_FCS_SHIFT		16
#define	TRANS_FCS_VAL(x)	((x & TRANS_FCS_MASK) >> TRANS_FCS_SHIFT)

#define	TRANS_NFCS_MASK		0x00020000
#define	TRANS_NFCS_SHIFT	17
#define	TRANS_NFCS_VAL(x)	((x & TRANS_NFCS_MASK) >> TRANS_NFCS_SHIFT)

#define	TRANS_UNUSED_MASK	0x80fc0000
#define	TRANS_UNUSED_SHIFT	0
#define	TRANS_UNUSED_VAL(x)	((x & TRANS_UNUSED_MASK) >> TRANS_UNUSED_SHIFT)

				/* CUG info				*/
#define	TRANS_CUG_MASK		0x7f000000
#define	TRANS_CUG_ZERO		0x64000000	/* Value of 00		*/
#define	TRANS_CUG_SHIFT		24
#define	TRANS_CUG_VAL(x)	((x & TRANS_CUG_MASK) >> TRANS_CUG_SHIFT)

typedef struct  net_site {
	char    *net_name;      /* name of network */
	char    *ts29_addr;     /* address for Ts29 TG connections */
	char    *x29_addr;      /* address for x29 TG connections */
	int     n_ftptrans;     /* max number of transfers/TS connection */
	int     n_ftpgates;     /* number of gateways to go through (NIFTP) */
	char    *ftp_addr;      /* address for NIFTP connections */
	int     n_mailgates;    /* number of gateways to go through for mail*/
	char    *mail_addr;     /* address for Mail connections */
	int     n_jtmpgates;    /* number of gateways to Jtmp */
	char    *jtmp_addr;     /* jtmp address */
	int	n_newsgates;	/* number of gateways to news */
	char	*news_addr;	/* news address */
	char	*gate_addr;	/* gateway replacement value */
	} net_entry;


struct  host_entry      {
	char    *host_name;     /* name of host */
	char    *host_alias;    /* alias of host */
	char    *host_info;     /* info about a host */
	long    n_timestamp;    /* time entry was added in NETWORK order */
	int     n_context;      /* don't ask me what this is.... */
	int     h_number;       /* Numeric host number */
	char    n_localhost;    /* is this a local host ? */
	char    n_oldhost;      /* is this from the old hosts file */
	char    n_disabled;     /* is this host disabled */
	int     n_nets;         /* number of network entries */
	net_entry  n_addrs[MAXNETS];    /* net site entries */
	};

struct  host_entry      *dbase_get();
struct  host_entry      *dbase_find();

/*
 * structure used to deal with domain entries
 */

typedef struct  {
	char    *domin_name;
	char    *domin_host;
	} domin_entry;

domin_entry    *domin_get();

/*
 * When scanning the UAIEF files the following letters are used:-
 * 'h' - a host entry
 * 's' - old serc aliases
 * 'r' - old relays
 */

/*
 * various network options
 */

#define N_LOCAL 1               /* local network */
#define N_NFCS	2               /* Do NOT use fast call select */
#define N_FCS	4               /* Do     use fast call select */

struct  NETWORK {
	char    *Nname;         /* name of network */
	char    *Nqueue;        /* name of queue for network */
	char    *Naddr;         /* address format for network */
	int     Nloglevel;      /* logging level of network */
	int     Nopts;          /* various option flags */
	char    *Nshow;         /* a name for putting in error msgs */
	int	Npkt_size;	/* default Packet size */
	int	Nwnd_size;	/* default Window size */
};

struct	NETALIAS {
	char	*NA_alias;	/* another name for this */
	char	*NA_realname;	/* the preffered name */
};

struct  DISKFULL {
	char    *Ddevice;       /* name of network */
	int     Dtype;          /* Type FTP, MAIL, NEWS, JTMP, PP, etc */
	int     Dbytes;         /* free bytes needed */
	int     Dpercent;       /* free percent */
};

/*
 * structure used to describe a listening channel
 */

#define L_REJECT   1    /* always reject messages */
#define L_COMPLAIN 2    /* complain about duff addresses */
#define L_CREJECT  4	/* reject duff calling strings */
#define L_REVERSE  8	/* enforce reverse lookups */
#define	L_ALLOW_TJI 16	/* Allow Take Joob Input */

struct  LISTEN  {
	char    *Lname;
	char    *Laddress;
	char    *Lchannel;
	int     Llevel;
	char    *Lprog;
	char    *Llogfile;
	char    *Lstatfile;
	char    *Lerrfile;
	char    *Linfomsg;
	int     Lopts;
	char    Lnchar;         /* name of net to be added to r_adr_trans */
};

struct  QUEUE   {
	char    *Qname;
	char    *Qdir;          /* name of the queue directory */
	int     Qlevel;
	char    *Qprog;
	char    *Qstatfile;
	char    *Qlogfile;
	char    *Qerrfile;
	char    *Qdbase;        /* channel specific database */
	char	*Qmaster;	/* master queue for this queue */
	int	Qordered;	/* xfers to be strictly time ordered */
	int	Qbackoff;	/* how quick to backoff (in secs) */
	int	Qbackmax;	/* how quick to backoff (in secs) */
};

/*
 * Name of the NRS tailor file
 */

#ifndef NRSTAILOR
#define NRSTAILOR	"/etc/niftptailor\
\0<****This area available for patching****>"
#endif  NRSTAILOR

/*
 * to deal with multiple domains / channels there are two tables
 */

extern  char    *NRSnetworks[]; /* names of all currently known networks */
extern  char    *NRSdomains[];  /* names of all currently known domains */
extern  char    *NRSqueues[];   /* names of all currently known queues */
extern  char    *NRSdbase;      /* name of NRS database */
extern  char    *NRSdqueue;     /* directory for queues */
extern  char    *NRSdspooler;   /* name of spooler signal file */
extern  char    *BINDIR;        /* default directory for binarys */
extern  char	*LOGDIR;	/* default directory for log files */
extern  char	*MAILDIR;	/* directory for incomming mail files */
extern  char	*DOCKETDIR;	/* directory for Q dockets */
extern  char    *SETUPPROG;     /* setup program name */
extern  char    *KEYFILE;       /* file containing secret info */
extern  char    *KILLSPOOL;     /* programme to wakeup ftpspool */
extern  char    *SECUREDIRS;    /* file containing the secure directories */
extern  char	*JTMPdir;	/* directory containing jtmp jobs */
extern  char    *JTMPproc;      /* name of jtmp request processor */
#ifdef NEWS
extern  char	*NEWSdir;	/* directory containing news jobs */
extern  char	*NEWSproc;	/* name of news request processor */
extern  char	*NEWSuser;	/* login id for news system interface */
extern  int	NEWSuid;
#endif NEWS
#ifdef PP
extern  char	*PPdir;		/* directory containing pp jobs */
extern  char	*PPchan;	/* chanel name to pass to pp */
extern  char	*PPproc;	/* name of pp request processor */
extern  char	*PPuser;	/* login id for news system interface */
extern  int	PPuid;
#endif PP
extern  struct  DISKFULL DISKFULLS[];	/* array of diskfull info */
extern  struct  NETWORK NETWORKS[];	/* array of networks */
extern  struct  NETALIAS NETALIASES[];	/* array of aliases */
extern  struct  LISTEN  LISTENS[];      /* array of listeners */
extern  struct  QUEUE   QUEUES[];       /* array of queues */
extern  char	*FTPuser;	/* login id for mailing ftp status messages */
extern  int	FTPuid;
#ifdef MAIL
extern  char	*MAILuser;	/* login id for mail system interface */
extern  int	MAILuid;
#endif MAIL
#ifdef JTMP
extern  char	*JTMPuser;	/* login id for jtmp system interface */
extern  int	JTMPuid;
#endif JTMP
extern	char	*altpwfile;	/* name of alternate password file */
extern	char	*banned_users;	/* List of banned users */
extern	char	*banned_file;	/* file of banned users */
extern	char	*guest_name;	/* name of "guest" account*/
extern  char	*mailprog;	/* /bin/mail equivalent for error messages */
extern  char	*printer;	/* line printer command (printf format) */
extern  int	Qretries;	/* retries before abandoning transfer */
extern	int	uselongform;	/* Should I use the long form ? */
extern  long	Qtimeout;	/* time in queue before abandoning */
extern	int	def_wndsize;	/* Default window size */
extern	int	def_pktsize;	/* Default packet size */
