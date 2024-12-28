.NH
Requirements
.LP
You will need some disk space, at least one network interface, and a few
days to understand and install \*(uN.
.NH 2
Disk space
.LP
The source distribution takes around 1.1\ MB in tar form. When unpacked and
compiled it takes around 6.5\ MB (the exact figure depends on the distribution
and the number of network interfaces you configure).
.LP
The installation puts files in several places:
.TS
center allbox tab(@) ;
l l l .
Files@Typical location@Approximate size
User binaries@/usr/local@0.7 MB
System binaries@/usr/local/lib/niftp@2.5 MB
Spool areas@/usr/spool/niftp@5 MB
.TE
.LP
The figure for spool area includes a copy of the NRS database, but as the
dbm(3) files are built in this area and moved into place you will need
at least 3MB of headroom. Mail files are also held here awaiting transmission
or processing, so an overall 10\ MB might be a better estimate for spool space.
.LP
The NRS (Name Registration Scheme) database is almost essential if you are
in the UK academic community. It is less useful or irrelevant in other cases.
If using NRS, allow space for the raw (DERFIL) form and for the nrs processing
software: more about this later.
.NH 2
Network interfaces
.LP
\*(uN will work with a wide range of network interfaces. It is possible to
configure several interfaces at once. The list of available interfaces is
continuously growing: contact the mailing list for the latest information.
Most of the interfaces are based on X25, but the code will work with any
reasonable full-duplex network.
The current list of interfaces follows.
.NH 3
Sun
.LP
SunLink-X25 (Sun part number 700-1190-04, manual 800-1333-04) allows either
serial port on a Sun processor board to act as an X25 synchronous interface.
It can also be used with the SunLink Communication Processor board.
Performance is quite adequate at 9600 Baud with the on-board serial port;
this could probably be pushed to 19200 Baud without serious problems.
.NH 3
X25B
.LP
This allows any machine with access to TCP/IP services on a network to
make calls out on X25 through a `bridge' machine. The bridge will run on
any machine which runs the "spad" program (contact pb@uk.ac.cam.cl
for details).
.NH 3
UBC
.LP
The University of British Columbia have produced a socket-based
implementation of X25 as part of their EAN mail system.
This is known to run on DEC VAX using a DMF-32 comms board and SUNs using
the zss lines. It may run on other 4.2Bsd-like systems.
Contact John Demco <demco@ean.ubc.cdn> for information on licensing.
Contact William Armitage <wja@cs.nott.ac.uk> for UK implementation info.
The distribution is in the form of a set of sources to implement the 'CCITT'
network in the kernel.
.NH 3
Internet
.LP
If your machine supports Berkeley sockets, you already have this network
interface. It can be used over ethernet or anything else that your IP
network runs on.
.br
If you have TCP connectivity, why not use the standard TCP protocols ?
.NH 3
IPCS
.LP
This is a UCL special -- you probably do not want it.
.NH 3
Dexpand
.LP
Dexpand is made by Camtec Electronics, and is an X25 (or Cambridge Ring)
interface for DEC VAX machines. You will need the Z1 software
supplied by the manufacturers.
.br
If you have an option, do not use this interface.
.NH 3
York
.LP
\*(uN can use the YorkBox Front End Processor, thus replacing a large part
of the original York code. It can also use any network interface which
has the same procedural interface as the YorkBox. This includes many
manufacturers X25 offerings.
.LP
To use the York interface, you will need the netio interface library.
.br
