.TH FTPQ 1 "UCL local 26/6/84"
.SH NAME
ftpq \- interogate the file transfer queue.
.SH SYNOPSIS
.B ftpq
[
.B \-lmadf -q <queue> host id...
]
.PP
.SH DESCRIPTION
.PP
This program allows a user to interrogate the file transfer
queues in a sane and civilized manner.
.PP
.IR Ftpq
will give a short summary of all jobs currently waiting to be
transferred for that user.
The information given is:-
.TP 5
.B 1)
The job's present position in the queue.
.TP 5
.B 2)
The local filename to be used in the transfer.
.TP 5
.B 3)
The current status of the transfer.
This is one of:-
.RS
.TP 10
.B "pend \-"
the transfer has not been tried yet.
.TP 10
.B "re-q \-"
the transfer has been tried but has failed
non-catastrophically.
It will be tried again later.
.TP 10
.B "try  \-"
the transfer\/connection is in the negotiation phase.
.TP 10
.B "go   \-"
You're in luck !
The file is currently being transferred.
.TP 10
.B "done \-"
The transfer has been completed successfully.
.TP 10
.B "ab   \-"
The transfer has been aborted in such a state that the
system cannot retry the transfer.
.TP 10
.B "rej  \-"
The transfer has been rejected for some reason.
.RE
.PP
Normally only the transfers that are
.IR viable
are shown.
The
.IR \-d
flag will also show the transfers that have completed.
I.e.
.IR done, aborted
or
.IR rejected.
.PP
The
.IR \-f
flag causes the recorded sender to be printed instead of the user and remote
filename.
Only available to root.
.PP
The
.IR \-l
flag gives a longer output listing.
If repeated, it guarentees that no information will be lost.
The additional information given is:-
.TP 5
.B 1)
The user performing the transfer.
.TP 5
.B 2)
The filename on the remote host.
.TP 5
.B 3)
The address of the remote host.
( As used by
.IR cpf )
.TP 5
.B 4)
The access mode of the transfer.
These can be one of:-
.br
mo   \- Make Only ( default ).
.br
ro   \- Replace Only.
.br
rom  \- Replace or make.
.br
ao   \- Append Only.
.br
aom  \- Append or make.
.br
tjo \- Take job output. Send to remote line printer.
.br
tji \- Take job input. Input a job to be executed.
.br
rdo   \- ReaD Only (get).
.br
dr  \- Destructive Read. Get then delete the file.
.TP 5
.B 5)
Wether the local file should be deleted at the end of the
transfer.
.PP
The
.IR \-a
flag gives information about all users\' transfers.
Normally only the current users transfers are shown.
.PP
The \-m
flag gives information about what seem to be
.IR mail
transfers.
.PP
The
.IR \-q
flag restricts listings to only the queue
.IR <queue>.
.PP
Any other arguments are taken to mean a remote host or specific message ids.
If any are specified then the search is restricted to only
the given host and message ids.
.PP
It is a feature of the current implementation that jobs that
are being tried show up at the bottom of the list.
.SH "SEE ALSO"
cpf(1), ftprm(1)
.SH DIAGNOSTICS
Mostly self-explanatory.
.SH FILES
/usr/spool/ftp/*, /lib/ftp/*.
.SH BUGS
The
.IR posn
value is somewhat arbitrary.
Only the first 150 items in the queue are checked,
this can give strange results.
.SH AUTHOR
Phil Cockcroft.
.br
Piete Brooks <pb@uk.ac.cam.cl>
