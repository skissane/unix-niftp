.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/intro.ms,v 5.5 90/08/01 13:32:33 pb Exp $
.\"
.\" $Log:	intro.ms,v $
.\" Revision 5.5  90/08/01  13:32:33  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:39:08  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:30  jpo
\" Initial revision
\" 
.\"
.NH
Introduction
.LP
This document comes in several parts. It is probably worth reading the
document through once before attempting to build and install \*(uN.
The process for installation is basically the following:
.IP \(bu
Unpacking the distribution. This is dependant obviously on how you
received the distribution.
.IP \(bu
Configuring the compile time options (if you have a source
distribution). This process is aided by the Configure script.
.IP \(bu
Installation of the software
.IP \(bu
Setting up the run time tailoring options.
.IP \(bu
Starting up the system.
.LP
Obviously some of these steps are more involved than others - but the
system is designed to be easy to install. 
.NH 2
Reporting Problems
.LP
The maintainers of this package are interested in feedback to do with
the software and installation.
If you find particular
difficulty or feel there is something missing either from the system
or the documentation please report it.
There is a distribution list of people involved in maintaining and
running the \*(uN package, if you would like to be added send a
message to the address:
.DS
unix-niftp-request@uk.ac.nott.cs
.DE
However, for reporting bugs, discussing problems and so on, the
appropriate address is
.DS
unix-niftp@uk.ac.nott.cs
.DE
Please include with your comments the following information.
.IP 1.
The version of \*(uN that you are running.
.IP 2.
The machine type you are running it on and the version of the
operating system.
.IP 3.
What sort of X.25 interface you are running and the version of that if
appropriate.
.LP
Well, with the commercial out of the way, we will take a look at the
configuration of the system.
