.TH DBENCODE 1L "unix-niftp"
.SH NAME
dbencode \- generate unix-niftp binary NRS database
.SH SYNOPSIS
.B dbencode
[database]
.BR < dbm-file
.SH DESCRIPTION
This generates a binary database from the intermediate format genarated
by the c-nrs programme.
Information on how to obtain this package is available by sending mail
to "info-server@uk.ac.cam.cl" with a Subject: line of
"Subject: Request catalogue; Topic c-nrs".
One of the make options is "make dbm1" which will create the desired
intermediate dbm-file.
.LP
It is also possible to collect the intermediate file from
UK.AC.NRS.DC-2.
.SH "SEE ALSO"
dbpatch(1), nrs(1)
.SH AUTHOR
Piete Brooks <pb@uk.ac.cam.cl>
