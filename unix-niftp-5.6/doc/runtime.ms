.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/runtime.ms,v 5.5 90/08/01 13:32:49 pb Exp $
.\"
.\" $Log:	runtime.ms,v $
.\" Revision 5.5  90/08/01  13:32:49  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:40:04  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:47  jpo
\" Initial revision
\" 
.\"
.NH
Run time configuration
.LP
Run time configuration allows the administrator to change the
behaviour of the system once compiled. The \*(uN package has the vast
majority of the important variables tailorable in this way. This means
that a change in the system usually does not require a recompilation.
.LP
To begin with it is worth looking at some of the standard run time
tailoring files supplied with the distribution. These are in the
directory samples/.
.LP
The best way to construct a tailor file is to take one of these that
closely resembles your system and modify the things you need to.
A full list of the possible option is listed below, together with some
idea of what they do, and how important they are.
First a quick note about the syntax of the file. 
.LP
Comments are allowed in the file, and start with a hash (#) and
terminate at the end of the line. Blank lines are ignored and may be
inserted to aid readability.
Most items occur in the form `Key value' or `Key List-of-values'.
.\" #### OK - check the above please someone.
.NH 2
Tailoring file keywords
.LP
A list of the correspondence between tailoring keywords and the
#defines that they are related to appears in Appendix 1.
However, this is a description of what the tailoring keywords control.
.IP ALIAS
.IP BINDIR
.IP DISKFULL
.IP DOCKETDIR
.IP DOMAIN
.IP DQUEUE
.IP FTPUSER
.IP HOST
Not currently used
.IP KEYFILE
.IP KILLSPOOL
.IP LISTEN
.IP LOGDIR
Directory to place log files in.
.IP LONGFORM
.IP MAILDIR
.IP MAILPROG
.IP MAILFMT
.IP NET
.IP OURNAME
.IP OUTDTYPE
.IP PADDRTYPE
.IP PRINTER
.IP QADDRTYPE
.IP QRETRIES
.IP QTIMEOUT
.IP QUEUE
.IP SECUREDIRS
.IP SETUPPROG
.IP SPOOLER
.IP TABLE
.IP JMTPDIR
.IP JTMPPROC
.IP JTMPUSER
.IP MAILUSER
.IP NEWSDIR
.IP NEWSPROC
.IP NEWSUSER
