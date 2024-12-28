Unix-niftp is the Grey Book engine for PP (THE mailer for UN*X).

The idea is that PP's own qmgr uses the unix-niftp code to do non-spooled
transfers, so that PP has full control over what is going on.

For incoming calls, the data is passed directly through to PP as it arrives.
This allows checking of the sender and host at the earliest possible stage.
[ However, this is not actually done -- sigh ! ]
Note however that invalid recipients are accepted at the NIFTP level,
with an error message send back later if necessary.
Invalid "From: " fields cause immediate rejection.

For outgoing calls, PP generates a queue entry & invokes a special P end
(called greyout) to try to send it, generating the data to transmit on the fly.

Note that both of these currently do not support to resumption, but there are
plans to add this to PP.


As PP is still in its early stages, it has been set up so that it can easily
be run in parallel with an existing mail system.
The normal method is to register a testing name which resolves to the YBTS
FTP.PP (instead of FTP.MAIL) for incoming calls.

If PP is to be the only mail system, include in niftptailor
	QADDRTYPE	ftp.mail,pp ftp,ftp
which means that the YBTS FTP.MAIL should use PP (and ftp is ftp!).

(Outgoing transfers are no problem)

There is a sample config.sh (a set of defaults for Configure) called
config.sh.sun4.pp which may be suitable for PP sites on SUN4s.
If you don't have an existing config.sh, you can
	cp config.sh.sun4.pp config.sh
and it should have reasonable defaults.


COMPILING:
==========

If PP was not selected when Configure was run, it is necessary to re-configure
the system. The simplest way is to delete the line starting "pp='" in
config.sh and type "make config" which will cause it to ask you about PP.

With PP defined, lib/pqproc/ will generate modules no_ppQspool.o and
no-ppQunspl.c which have dummy routines for the PP Q end code.
It also generates ppQspool.o which includes the spooled Q routines.
If the non spooled code is available from the PP file Chans/grey/ppQunsppol.o
it should be installed as lib/pqproc/ppQunspool.o. If this is not done,
the makefile will copy no-ppQunspl.c.

SO: make in the niftp src directory. Move to /usr/src/pp/Chans/grey/ and do a
    make there so as to install the pp .o's and make the real qsun (or qXX)
    and a greyout for PP.

TAILORING
=========

See samples/PP/nt.j.only.PP for a sample /etc/niftptailor for a Sun PP site.

The tailoring available in niftptailor is:
	LISTEN	A listener line MUST be supplied for PP to pick up the log
		level and channel name (for possible sunstitution in PPCHAN)
		Note that there is no default. A typical line might be:
		LISTEN lj address="FTP",level=107,channel=janet
	NET	The default window and packet sizes can be set either in
		pptailor (prog="greyout-Pp -P -p512 -w7 -q Pp"), or in
		niftptailor by setting the wnd and pkt variable for the NET
		(NET PP queue=Pp,address="000008010101/%E%X%D/%T",wnd=7,pkt=256
		Note that a UAIEF record can over-ride these values.
	PPUSER	the user to own PP transfers
		Default: pp
	PPCHAN	the channel name to pass to PP. It may contain a %s, which
		will be substitued with the channel name, as defined in LISTEN
		above. The result must be a channel name in the pp tailor file
		Default: gb-%s
	QADDRTYPE allow the system to know that this is a PP transfer.
		Default: ftp.pp,pp
	QUEUE	This tells the P end (sender) about log levels etc.
		There is NO default. A typical line might be:
		QUEUE   PP      level=111,prog=greyout

Users of my ybtsd on suns may need to add a line to /etc/ybts-auth:
/usr/lib/niftp/lj:ftp.pp:*
where the "lj" matches up with the LISTEN string above.


TESTING:
========

The PP logs should be found as per the PP manual.
The unix-niftp logs are in a directory such as /usr/spool/niftp/logs
(see DQUEUE in /etc/niftptailor).

The level of logging is a bitmask set by the "level=107" type statements in
the LISTEN (for Q end / incoming) and QUEUE (for P end / sending).
If things do not work, keep turning up the debug level.
Set it to "-1" (i.e. all). If there is too much logging, find the relevent bit
and unset it. E.g. the line
log.00008.19d5.001: sending  ProtiD      [==Int] (x)0100
means that it it a log (rather than warning or debugging) due to bit 0x8 being
set, for process 19d5 (in hex) 1 second after the last message ws generated.

If you have spad, then type "spad 00000801008057/FTP.PP" where 00000801008057
is the listening DTE and FTP.PP is the YBTS which should appear in QADDRTYPE
(and in /etc/ybts-auth in my ybtsd is used).
This will at least test that the basic bits of the unix-niftp listening code
is working.

Then you can try a test by creating a file /tmp/m such as:

postmaster@uk.ac.cam.cl

From: postmaster@cam.cl
Subject: testing

Body

and then use the command (as root) "cpf -t /tmp/m @cam.cl"
(where cam.cl is replaced by your site name).
If you are testing PP before putting it into service and you do not have a
name already registered in the NRS for testing, you can create a UAIEF record
to add a temporary entry. Create a file /tmp/UAIEF such as

h:uk.ac.cambridge.computer-lab.test:uk.ac.cam.cl.test:janet:4:00000801008079:
FTP.PP::::::

(which should all be one line with no spaces) and then type
"/usr/lib/niftp/dbpatch < /tmp/UAIEF" which will say something like:
"First host is 1981". Now the command "lookdbm uk.ac.cam.cl.test" should give:
Info for uk.ac.cam.cl.test:-
host name: uk.ac.cambridge.computer-lab.test
alias: uk.ac.cam.cl.test
host info: DE <not in the NRS>
Added at Thu Mar  1 06:20:21 1990
Number of nets = 1
Host number = 1982
network: janet
ts29:    NO TS29
x29:     NO X29
ftp:     NO FTP
Max trans/TS conn: 0
mail:    DT 00000801008079
YB FTP.PP
jtmp:    NO JTMP
news:    NO NEWS
gate:    NO GATEWAY





[[ Below is the OLD cruddy way of doing things -- shouldn't normally be used ]]

It is also possible to make the Q end work in a spooled manner, by
specifying PPPROC (in niftptailor) to be other than the default value of
"inline", in which case that procedure is called to process the file.
The arguments can be set including a MAILFMT line in niftptailor.

TAILORING
=========

	PPPROC	name of code to process incoming mail items.
		"inline" means that the non-spooled method should be used
		otherwise it is the path of a procedure to submit a JNT format
		mail item into PP.
		Default: inline
	PPDIR	the directory for spooling incoming PP files, if PPPROC is not
		"inline".
		Default: /usr/spool/niftp/pp
	MAILFMT	this can be used to change the options passed to PPPROC.
		The arguments to the sprintf statement are: PPPROC,
			{ host, channel, file } * 3
		They are repeated, so that by using %0.0s the items can be
		picked up in any order (not used if PPPROC is "inline").
		Default: <prog> grep <host> <file>	SENDASIS | CATCHALL
#MAILFMT  ni_pp "Never Used", "%s grey %s %0.0s%s", 48
