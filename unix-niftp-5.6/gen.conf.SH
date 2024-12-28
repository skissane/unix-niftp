case $CONFIG in
    '') . config.sh ;;
esac
case "$bannedusers" in
''|' ')	BANNEDUSERS="(char *) 0";;
*)	BANNEDUSERS="\"$bannedusers\"";;
esac
case "$bannedfile" in
''|' ')	BANNEDFILE="(char *) 0";;
*)	BANNEDFILE="\"$bannedfile\"";;
esac
case "$guestname" in
''|' ')	GUESTUSER="\"guest\"";;
*)	GUESTUSER="\"$guestname\"";;
esac
case "$altpwfile" in
''|' ')	ALTPWFILE="(char *) 0";;
*)	ALTPWFILE="\"$altpwfile\"";;
esac
outputfile=lib/gen/conf.c
rm -f $outputfile.$$
echo "Creating $outputfile (with variable substitutions)"
if cat >$outputfile.$$ <<!GROK!THIS!
/* unix-niftp lib/gen/conf.c $Revision: 5.0 Date: 87/03/23 03:35:16 */

/*
 *	This file contains the configuration dependent data structures.
 *	Most but not all are dynamicaly configurable by the code in tailor.c
 *
 *	$Log: gen.conf.SH,v $
Revision 5.6.1.1  1992/10/17  06:18:56  pb
add ftp_print

Revision 5.6  1991/06/07  17:00:01  pb
Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites

Revision 5.5  90/08/01  13:28:57  pb
Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board

Revision 5.3  89/07/16  12:01:36  pb
Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q

Revision 5.2  89/01/13  14:31:10  pb
Distribution of Jan89SuckMail: Support Sucking of mail

Revision 5.1  88/10/07  17:30:30  pb
Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned

 * Revision 5.0.1.3  87/12/09  16:20:31  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
Revision 5.0  87/09/28  13:45:37  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:35:16  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 */

#include "ftp.h"
#include "nrs.h"
#include "hcontrol.h"

/* the default directory for binaries		BINDIR		*/
char	*BINDIR = "$privatebin";

/* directory for storing Q process dockets	DOCKETDIR	*/
char	*DOCKETDIR = "$spool/dockets";

/* currently known domains (order important)	DOMAIN		*/
char    *NRSdomains[CONF_MAX] = { "uk.ac.","uk.co.",".",".ac.uk",".co.uk",0,};

/* the default queue stem			DQUEUE		*/
char	*NRSdqueue = "$spool";

/* the name of the keyfile.			KEYFILE		*/
char	*KEYFILE = "$privatebin/key/keyfile";

/* the name of the programm to wakeup ftpspool.	KILLSPOOL	*/
char	*KILLSPOOL = "$privatebin/killspool";

/* the default directory for logfiles		LOGDIR		*/
char	*LOGDIR = "$spool/logs";

/* directory for receiving mail files		MAILDIR		*/
char	*MAILDIR = "$spool/mail";

/* How to notify local users by mail		MAILPROG	*/
char	*mailprog = "/bin/mail";

/* the format for the above ... */
/* Name, how to send local, how to deliver remote, bits */
struct mailfmt mailfmt[CONF_MAX] = {
 { "v6mail",	"%s -g \"NIFTP Daemon\" -s \"%s\" -t \"%s\"", "",
		ESCAPE_USER | ESCAPE_SUBJECT },
 { "sendmail",	"%s -t", "%s -ba -oi -oMs%s", ADD_TO | ADD_SUBJ | ADD_BLANK},
 { "/usr/ucb/mail", "%s -s \"%s\" \"%s\"", "%s -s 'Received from %s'",
		ADD_RECV | ESCAPE_USER | ESCAPE_SUBJECT },
 { "mail",	"%s -s \"%s\" \"%s\"", "%s -s 'Received from %s'",
		ADD_RECV | ESCAPE_USER | ESCAPE_SUBJECT },
 { "hhmail",	"%s \"%0s%s\"",	"",
	ADD_TO | ADD_SUBJ | ADD_BLANK | ADD_RECV| ESCAPE_USER },
 { "progmail",	"%s -s \"%s\" \"%s\"", "%s -v '%s'",
		ADD_RECV | CATCH_ALL | ESCAPE_USER | ESCAPE_SUBJECT },
#ifdef	PP
 { "ni_pp",	"Never Used", "%s grey %s %0.0s%s",
		CATCH_ALL | SEND_ASIS },
 { "greyin",	"Never Used", "%s grey %s %0.0s%s",
		CATCH_ALL | SEND_ASIS },
#endif	PP
 { "asismail",	"%s -s \"%s\" \"%s\"", "%s -v '%s'",
		CATCH_ALL | SEND_ASIS | ESCAPE_USER | ESCAPE_SUBJECT },
};

/* the allowable printer types			OUTDTYPE	*/
char	*outdtype[CONF_MAX] = { "lp", 0, };

/* ourname					OURNAME		*/
char *ourname = "";

/* the printer program command			PRINTER		*/
char	*printer = "exec lpr %s %s";

/* alternate password file 			ALTPWFILE	*/
char	*altpwfile = $ALTPWFILE;

/* guest user account 	 			GUESTUSER	*/
char	*guest_name = $GUESTUSER;

/* list of banned users - default is none	BANNEDUSERS	*/
char	*banned_users = $BANNEDUSERS;

/* file of banned users - default is none	BANNEDFILE	*/
char	*banned_file = $BANNEDFILE;

/* currently known about queues			QUEUE		*/
char    *NRSqueues[CONF_MAX] = { "qu", "qj","qp", 0, };

/* the name of the secure directory file	SECUREDIRS	*/
char	*SECUREDIRS = "$privatebin/securedirs";

/* the name of the setup program		SETUPPROG	*/
char	*SETUPPROG = "$privatebin/setup";

/* the default spooler signal file		SPOOLER		*/
char	*NRSdspooler = "$spool/ftpspooler";

/* the default name of the NRS database		TABLE		*/
char	*NRSdbase = "$spool/nrsdbm";

#ifdef JTMP
/* name of the JTMP work directory		JTMPDIR		*/
char	*JTMPdir = "$spool/jtmp";

/* name of JMTP processing daemon.		JTMPPROC	*/
char    *JTMPproc = "djtmp";
#endif JTMP

#ifdef NEWS
/* name of the NEWS work directory		NEWSDIR		*/
char	*NEWSdir = "$spool/news";

/* name of NEWS processing daemon.		NEWSPROC	*/
char	*NEWSproc = "rnews";
#endif NEWS

#ifdef PP
/* name of the PP work directory		PPDIR		*/
char	*PPdir = "$ppspooldir";

/* name of PP processing daemon.		PPPROC	*/
char	*PPproc = "$ppproc";

/* name of PP channel.				PPCHAN	*/
char	*PPchan = "$ppchan";
#endif PP

/* array of structures for networks		NET		*/
struct  NETWORK NETWORKS[CONF_MAX] = {
	{ "janet", "qj", 0, 0, 0, 0, 0, 0 },
	{ "pss",   "qp", 0, 0, 0, 0, 0, 0 },
};

/* Array of aliases for networks				*/
struct	NETALIAS NETALIASES[CONF_MAX*5] = { { "serc", "janet"} };

/* array of structures for testing full disks	DISKFULLS	*/
struct  DISKFULL DISKFULLS[CONF_MAX * 3];

char *FTPuser	= "daemon";	int FTPuid = 1;		/* FTPUSER	*/
#ifdef MAIL
char *MAILuser	= "mmdf";	int MAILuid = 67;	/* MAILUSER	*/
#endif MAIL
#ifdef JTMP
char *JTMPuser	= "root";	int JTMPuid = 0;	/* JTMPUSER	*/
#endif JTMP
#ifdef NEWS
char *NEWSuser	= "mmdf";	int NEWSuid = 211;	/* NEWSUSER	*/
#endif NEWS
#ifdef PP
char *PPuser	= "pp";		int PPuid = 211;	/* PPUSER	*/
#endif PP

/* Do we want to use the ridiculous long form	LONGFORM	*/
int uselongform = $longform;
/*
 * limits to the number and duration of attempts to transfer file
 * currently these are global, should be per channel.
 */
int	Qretries = QRETRIES;	/*		QRETRIES	*/
long	Qtimeout = QTIMEOUT;	/*		QTIMEOUT	*/

#ifndef	DEF_WNDSIZE
#define	DEF_WNDSIZE	0
#endif	/* DEF_WNDSIZE */

#ifndef	DEF_PKTSIZE
#define	DEF_PKTSIZE	0
#endif	/* DEF_PKTSIZE */

/* default window and packet sizes */
int	def_wndsize	= DEF_WNDSIZE;
int	def_pktsize	= DEF_PKTSIZE;

#ifndef	DEF_FTP_PRINT
#define	DEF_FTP_PRINT	0
#endif
/* so that it's defined .... */
int	ftp_print	= DEF_FTP_PRINT;

/* NOT YET TAILORED -------------------------------------------	*/
/*
 * P process type -> extension mapping
 * this should be tailored (on a per channel basis)
 */

struct addrtype p_addrtype[CONF_MAX*5] = {
	"FTP",		T_FTP,
	"FTP.MAIL",	T_MAIL,
#ifdef JTMP
	"FTP.JTMP",	T_JTMP,
#endif JTMP
#ifdef NEWS
	"FTP.NEWS",	T_NEWS,
#endif NEWS
#ifdef PP
	"FTP.MAIL",	T_PP,
#endif PP
	(char *)0,	0,
};

/*
 * Q channel address -> type mapping
 * this too should be tailored on a per channel basis
 */

struct addrtype q_addrtype[CONF_MAX*5] = {
	"ftp",		T_FTP,
	"ftp.mail",	T_MAIL,
#ifdef JTMP
	"ftp.jtmp",	T_JTMP,
#endif JTMP
#ifdef NEWS
	"ftp.news",	T_NEWS,
#endif NEWS
#ifdef PP
	"ftp.pp",	T_PP,
#endif PP
	"niftp",	T_FTP,
	"niftp.mail",	T_MAIL,
#ifdef JTMP
	"niftp.jtmp",	T_JTMP,
#endif JTMP
#ifdef NEWS
	"niftp.news",	T_NEWS,
#endif NEWS
#ifdef PP
	"niftp.pp",	T_PP,
#endif PP
	(char *)0,	0,
};

char    *NRSnetworks[CONF_MAX] = { "janet", "pss", 0, };

/* array of structures for listeners		---		*/
struct  LISTEN  LISTENS[CONF_MAX];

/* array of structures for queues		---		*/
struct  QUEUE   QUEUES[CONF_MAX];
!GROK!THIS!
then if cmp $outputfile $outputfile.$$ > /dev/null 2>& 1
	then
		rm -f $outputfile.$$
		$echoq "	" $outputfile not affected
	elif mv $outputfile.$$ $outputfile
	then
		echo "	" $outputfile updated
	else
		echo "+++	" $outputfile left in $outputfile.$$
	fi
else
	echo "	" Failed to create $outputfile.$$
fi
