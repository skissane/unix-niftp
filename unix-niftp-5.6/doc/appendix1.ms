.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/appendix1.ms,v 5.5 90/08/01 13:32:18 pb Exp $
.\"
.\" $Log:	appendix1.ms,v $
.\" Revision 5.5  90/08/01  13:32:18  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:38:18  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:26  jpo
\" Initial revision
\" 
.\"
.SH
Appendix 1.
.LP
The relationship between the keywords found in the tailoring file and
the #defines used at compile time is summarised here.
.TS
center expand tab(@) ;
l l l .
Tailor Keyword@#define@Usage

ALIAS@NETALIASES@
BINDIR@BINDIR@
DISKFULL@DISKFULLS@
DOCKETDIR@DOCKETDIR@
DOMAIN@NRSdomains@
DQUEUE@NRSdqueue@
FTPUSER@FTPuser FTPuid@
HOST@@
KEYFILE@KEYFILE@
KILLSPOOL@KILLSPOOL@
LISTEN@NULL@
LOGDIR@LOGDIR@
LONGFORM@uselongform@
MAILDIR@MAILDIR@
MAILPROG@mailprog@
MAILFMT@MAILFMT@
NET@NETWORKS@
OURNAME@ourname@
OUTDTYPE@outdtype@
PADDRTYPE@p_addrtype@
PRINTER@printer Q@
QADDRTYPE@q_addrtype@
QRETRIES@QRETRIES Q@
QTIMEOUT@QTIMEOUT Q@
QUEUE@NRSqueues Q@
SECUREDIRS@SECUREDIRS@
SETUPPROG@SETUPPROG@
SPOOLER@NRSdspooler@
TABLE@NRSdbase@
JMTPDIR@JTMPDIR@
JTMPPROC@JTMPproc@J
JTMPUSER@JTMPuser (uid)@J
MAILUSER@MAILuser (uid)@J
NEWSDIR@NEWSdir@M
NEWSPROC@NEWSPROC@N
NEWSUSER@NEWSuser (uid)@N
.TE
