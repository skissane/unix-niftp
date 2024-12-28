.TH FTSHOW 1L "unix-niftp"
.SH NAME
ftshow \- show active unix-niftp processes
.SH SYNOPSIS
.B ftshow [\-q]
/usr/spool/niftp/logs/ST*
.SH DESCRIPTION
This programme shows the state of the selected unix-niftp processes.
ST* shows all process, ST.q* shows all Q processes, etc..
.LP
On systems which have permanent listeners (rather than a generic
listener which forks the required Q process when a call arrives) the Q
processes should always be visible.
The P processes are only visible when active.
.LP
The format of the output is queue name, process number, seconds in this
state/seconds since last dead mans handle, bytes transfered, current
state, call address. E.g.
.br
qjl1 p123 t30 /4 v7172 DATA =uk.ac.cam.cl.test (FTP,NRS.FTP)
.br
indicates that the Q process 123 is listening on FTP and NRS.FTP for the
queue called jl1 and has accepted a call from uk.ac.cam.cl.test which
has been in the DATA state for 30 seconds and last updated the stats
file 4 seconds ago, having transferred 7172 byes of data.
.LP
If the time after the / is large, it is likely that the process has hung
and it should be killed.
.LP
The states are:
.TP 14
LISTEN
Q process listening for incoming call
.TP 14
DATA
in the transfer phase
.TP 14
DEAD
transfer is dead
.TP 14
OPENING
attempting to make an outgoing call
.TP 14
DECODING
decoding an address
.TP 14
IDLE
not doing anything \- file is probabably spurious
.TP 14
ERRCLOSE
closing a connection after an error
.TP 14
LISTENFAILED
a listen has just failed
.TP 14
SFT
waiting for an SFT (Start File Transfer)
.TP 14
DONE
has just completed a transfer
.TP 14
AWAITSTOP
awaiting a stop from the other end
.TP 14
PROCESS
processing a transfered file
.TP 14
PROCESSED
processed a transferred file
.TP 14
OPENFAILED
an open attempt has just failed
.TP 14
FINDP
looking for a file to transfer
.TP 14
FOUNDP
found a file to transfer
.TP 14
RELISTEN
relistening after a listen error
.TP 14
FAILLISTEN
listening is repeatedly failing
.SH "SEE ALSO"
ftpq(1)
.SH AUTHOR
Piete Brooks <pb@uk.ac.cam.cl>
