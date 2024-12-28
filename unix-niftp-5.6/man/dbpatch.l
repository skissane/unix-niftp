.TH DBPATCH 1L "unix-niftp"
.SH NAME
dbpatch \- add UAIEF records to unix-niftp NRS database
.SH SYNOPSIS
.B dbpatch
[database]
.BR < UAIEF-file
.SH DESCRIPTION
This installs a number of patches to the NRS database which for some
reason are not held in the NRS itself. The resaon may be
.IP
The context is not defined in the NRS.
For example FTP.NEWS has been allocated context 127
.IP
The network is nor defined in the NRS.
For example ethernet hosts using the inet interface.
.IP
The information cannot be registered in the NRS.
For example a number of hosts have an implementation of NIFTP which only
expects a single file transfer per call.
.IP
The host is not yet in the NRS.
As it takes some time to become registered, it is useful to enter in
host information immediately.
.IP
The entry is to perform a reverse mapping through a gateway.
.IP
The information is private and is meant not to be in the NRS.
.IP
To test out special addresses.
For example to test a new system.
.IP
To allow a site to fetch a DERFIL2 to generate a full NRS database.
dbpatch will not create a database, so in this case it is necessary to
create empty .dir and .pag files, e.g. "touch
/usr/spool/niftp/nrsdbm.dir /usr/spool/niftp/nrsdbm.pag".
.LP
If more control is wanted over the flow of news, the P and Q processes
can be processed separately from the normal transfers.
The Q (listening) can be set up by having niftp and niftp-mail handled
by one (or multiple) listeners and NEWS by another.
On systems which listen for an explicit YBTS, such a a standard YorkBox,
this is simply a matter of saying
.br
LISTEN lj address="FTP,FTP.MAIL",channel=janet,prog=qyork
.br
LISTEN lk address="FTP,FTP.MAIL",channel=janet,prog=qyork
.br
LISTEN lu address="FTP.NEWS",channel=janet,prog=qyork
.br
which will allow lu to be enabled and disables separately and will avoid
news transfers blocking niftp and niftp-mail.
On systems which accept calls which have a specified leading substring,
such as the dexpand where listening on FTP will accept calls for
FTP.MAIL as well as just FTP, it is necessary to tell your newsfeeds
that your address some other string that does not start with the same
substring, e.g.
.br
LISTEN lu address="NEWS.FTP",channel=janet,prog=qdex
.br
in which case it is also important to tell the system the actual
transfer type assosiated with NEWS.FTP, e.g.
.br
QADDRTYPE  ftp,ftp, ftp.mail,mail, ftp.news,news, news.ftp,news, news.ftp.news,news
.br
noting that it is safer to have both NEWS.FTP and NEWS.FTP.NEWS
registered, just in case the other end automatically appends a .NEWS.
.PP
For the P (sending) end, it is possible to register all news (context
127) entries on a separate (pseudo) network "news" (rather than janet or
pss) and have duplicate QUEUE and NET entries, e.g.
.br
QUEUE qj level=127,prog=pdex
.br
QUEUE qu level=127,prog=pdex
.br
NET janet queue=qj,address="000008013177/%E%XB%D/%T"
.br
NET news  queue=qu,address="000008013177/%E%XA%D/%T"
.br
again allowing separate control of when the transfer are tried (e.g.
only during the night) and avoiding blocking more important transfers.
.SH "SEE ALSO"
dbencode(1)
.SH AUTHOR
Piete Brooks <pb@uk.ac.cam.cl>
