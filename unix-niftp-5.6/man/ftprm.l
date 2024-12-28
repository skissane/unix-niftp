.TH FTPRM 1 "UCL local 26/6/84"
.SH NAME
ftprm \- delete jobs from the ftp queue.
.SH SYNOPSIS
.B ftprm
[
.B \-mal \-q <queue> \-u <user>.. host id...
]
.PP
.SH DESCRIPTION
.IR Ftprm
allows a user to delete jobs from the ftp queues.
With no flags
.IR ftprm
will delete all jobs owned by the user.
.PP
The
.IR \-q
flag restricts deletions to only those in the queue <queue>.
.PP
The
.IR \-u
flag restricts deletions to only user <user>.
Up to 100 users may be specified by using
.IR "\-u user1 \-u user2"
constructs.
.PP
The
.IR \-a
flag deletes entries for all users.
This flag is only acted upon when invoked by the super-user.
.PP
The
.IR \-m
flag deletes entries that look as though they are mail.
Normally these entries are not deleted.
.PP
If any more arguments are given then these are taken to mean
a specific hostname or a list of queue ids.
If both are specified, the intersection is processed.
.PP
.SH "SEE ALSO"
ftpq(1), cpf(1)
.SH DIAGNOSTICS
Self-explanatory.
.SH FILES
~/.confftp, /usr/spool/ftp/*, /lib/ftp/*.
.SH BUGS
Should be able to specify jobs via the
.B POSN
number.
.SH AUTHOR
Phil Cockcroft.
.br
Piete Brooks <pb@uk.ac.cam.cl>
