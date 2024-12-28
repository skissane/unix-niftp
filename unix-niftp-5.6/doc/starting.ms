.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/starting.ms,v 5.5 90/08/01 13:32:54 pb Exp $
.\"
.\" $Log:	starting.ms,v $
.\" Revision 5.5  90/08/01  13:32:54  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:40:13  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:53  jpo
\" Initial revision
\" 
.\"
.NH
Starting up the system
.LP
This area is divided into two. There are a few once only things that
have to be done to allow the system to be run, and some things that
must be done each time a new version is installed.
.NH 2
Once only actions
.LP
The once only actions are as follows.
Normally, you will want the \*(uN system to start running
automatically when your system reboots. To achieve this it is
necessary to add the following lines to your /etc/rc.local file.
.DS
if [ -f /pathname/ftpspool ]; then
	/pathname/ftpspool logfile & (echo -n ' ftpspool') >/dev/console
fi
.DE
You will need to supply the correct pathname depending upon where you
installed ftpspool.
Also check that you have a dbm database.
.NH 2
Each installation changes.
.LP
On each reinstallation you may need to change the following...
