.TH CPF 1L "unix-niftp"
.SH NAME
cpf \- copy files between different hosts.
.SH SYNOPSIS
.B cpf
.RB [ \-ntw
.BR \-a [ = ]mode
.BR \-b [ = ]number
.BR \-mdqsfA [ = ]string]
file1
.RB file2 @ host
.PP
.B cpf
.RB [ \-nt
.BR \-a [ = ]mode
.BR \-b [ = ]number
.BR \-mdqsA [ = ]string]
.RB file1 @ host
file2
.SH DESCRIPTION
.PP
This program provides a user interface to the
Network Independent File Transfer Protocol,
NIFTP-B(80).
Much of the terminology of this document is taken straight
from the Protocol Specification (The Blue Book).
.PP
In the first form,
the file
.I <file1>
is copied to
.IR <file2>
on the remote host
.I <host>.
Wherever a host name is expected, it can be replaced by channel and host, e.g.
janet/cam.cl.
.PP
In the second form, the file,
.I <file1>
is copied from the remote host,
.I <host>
into
.I <file2>
on the local machine.
.PP
The
.I host
parameter may be any one of the sites currently in the database
of local hosts.
.B Special
treatment is given to local host transfers.
.PP
For copying files between non-local sites
.B cpf
will ask for the username on the remote machine.
In all cases it will ask for the password on the remote
machine.
On local transfers this need not be your login
password, it can be the ftp password if one is set.
.PP
If the environment variable
.B SAVEFTP
is set then
.B cpf
will save a copy of the encrypted remote password in the file
.B \.confftp
in the user's home directory.
On subsequent transfers to the same remote host,
.B cpf
will not ask for a password (or username) but will get this information
from the .confftp file.
.PP
When
.I cpf
is copying a file
.I to
a remote host the transfer will fail if
.I file2
already exists, or, the user does not have sufficient permission to
create the file.
The user will be informed in this case.
This feature can be overridden by the use of the \-a flag.
.PP
All failures are reported via the
.I mail system.
.PP
.SH "Local transfers."
When transferring files between local
.I Unix
sites, special options are used.
These are:-
.TP
.B 1)
If the full pathname of a file on the remote machine is not
given then the file is accessed relative to the home directory of the
user on the remote machine.
.TP
.B 2)
The file transferred will retain the same access and modification
times as the original.
.SH "Optional flags."
.PP
The following flags are permitted:-
.TP
.B \-w
When the file has been transferred correctly the source file
will be deleted. Thus a
.B \`mv\'
command can be effected.
.TP
.B \-n
Inform the user when the transfer has completed successfully.
Normally only failed transfers are reported back to the user.
.TP
.B \-F
For sites that require file passwords (not needed on local
sites) this allows the password to be entered. The program
will prompt for the password.
.TP
.B \-f[=][name]
This flag is used by the mail system to specify the sender of
the mail so that the file transfer system can inform the user
of a failed mail transfer at the ftp layer.
On non mail transfers it has no effect.
.TP
.B \-b[number]
This flag specifies a binary mode of transfer.
A number can follow this flag specifying the word size to be used in the
transfer.
If no number is specified the default is assumed to be 8 bit words.
.br
To move binary files about local sites this flag need not be used.
.TP
.B \-a[=][mode]
This flag specifies the access mode to be used when transferring files.
These names are define in the Blue Book in Section 7.2.
This will not override the specification given by the placing of the
.B host
parameter.
By default the access mode is
.B makeonly
(\-amakeonly) for transfers to a remote site or
.B readonly
(\-areadonly) for transfers from a remote site.
.br
When the access mode is
.B makeonly
the transfer will fail if the destination file exists.
To overwrite a file use
.BR \-areplaceormake .
.RS .5i
The full list of modes are:-
.TP
.B \-amakeonly (-amo)
Send the file to the remote host only if it does not yet exists there.
.TP
.B \-areplaceonly (-aro)
Send the file to the remote host if it already exists there.
.TP
.B \-areplaceormake (-arom)
Send the file to the remote host whether or not it already exists there.
.TP
.B \-aappendonly (-aao)
Append the transferred file to the destination file.
If the destination file does not exist then the transfer will fail.
.TP
.B \-aappendormake (-aaom)
Append the transferred file to the destination file.
If the file does not exist create it.
.TP
.B \-atakejobinput (-atji)
Send a file from the local host and inject it into the remote Jobmill,
i.e. remotely execute a local file.
The remote filename, if any, is ignored.
.TP
.B \-atakejoboutput (-atjo)
Send a file from the local host to the output device on the remote host,
i.e. print a local file remotely.
The remote filename, if any, is ignored.
See 
.B \-d
and
.BR \-q .
.TP
.B \-areadonly (-ardo)
This is the default for receiving a file.
Note that the abbreviation -aro means replaceonly.
.TP
.B \-areadandremove (-arar)
This reads the file from the remote host and then the file on
the remote host is deleted.
If the deletion fails it is not guaranteed that the user will be informed.
.TP
.B \-adestructiveread (-adr)
This reads from the remote host deleting the read file progressively.
This is for talking to Card readers.
It is not supported on local hosts.
.TP
.B \-agivejobinput (-agji)
Read a file from the remote host and inject it into the local Jobmill,
i.e. localy execute a remote file.
The local filename, if any, is ignored.
.TP
.B \-agivejoboutput (-agjo)
?
.RE
.TP
.B \-t
Specifies that the file is to be sent as JNT Mail (if supported).
This suppresses certain checks and sets the username and filename
attributes to be null strings.
The specified file will
.B ALWAYS
be deleted.
This option is only permitted for privileged users.
.TP
.B \-J
Specifies that the file is to be sent as a JTMP job (if supported).
This suppresses certain checks and sets the username and filename
attributes to be null strings.
The specified file will
.B ALWAYS
be deleted.
This option is only permitted for privileged users.
.TP
.B \-N
Specifies that the file is to be sent as NEWS (if supported).
This suppresses certain checks and sets the username and filename
attributes to be null strings.
The 
.B \-w
flag should be used if the file is to be deleted.
This option is only permitted for privileged users.
.PP
The remaining flags allow certain transfer attributes to be set
as required;
The interpretation of these attributes is implementation-dependent.
.TP
.B \-U[=]<string>
Sets the username to <string>.
.TP
.B \-p[=]<string>
Sets the password to <string>.
If no username has been given then it is set to the current
users username.
.br
To set a null password use
.B \-p=
, and to set no password, use
.BR \-p .
.TP
.B \-m[=]<string>
Sets the Monitor message for the transfer to be <string>.
.TP
.B \-d[=]<string>
Sets the Output Device Type to be <string>.
.TP
.B \-q[=]<string>
Sets the Device Type Qualifier to be <string>.
.TP
.B \-s[=]<string>
Sets the Special Options to be <string>.
.TP
.B \-A[=]<string>
Sets the account information
.SH "SEE ALSO"
ftpq(1), ftprm(1)
.br
"Network Independent File Transfer Protocol", published by JNT
(often called `The Blue Book').
.SH DIAGNOSTICS
Mostly self-explanatory.
.SH FILES
~/.confftp
.SH BUGS
Should handle transfers involving many files in a better way.
.SH AUTHOR
Phil Cockcroft.
.br
Mods by Piete Brooks <pb@uk.ac.cam.cl>
