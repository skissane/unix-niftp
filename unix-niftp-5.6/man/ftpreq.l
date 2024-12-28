.TH FTPREQ 8L "unix-niftp"
.SH NAME
ftpreq \- re-queue entries for transfer
.SH SYNOPSIS
.B ftpreq
[
.B \-z
.BR \-k <keepsecs>
]
file ...
.PP
.SH DESCRIPTION
.IR Ftpreq
can reset the "next attempt" time for transfers and/or remove old docket files.
.PP
The
.IR \-k
flag causes gives thenumber of seconds to keep docket files
(default is an hour).
.PP
The
.IR \-z
flag causes all the "next attempt" fields of the specified fils
to be set to the current time.
.SH "SEE ALSO"
ftprm(1)
.SH DIAGNOSTICS
Self-explanatory.
.SH FILES
typically /usr/spool/niftp/q*/q*
.SH AUTHOR
Piete Brooks <pb@uk.ac.cam.cl>
