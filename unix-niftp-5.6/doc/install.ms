.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/install.ms,v 5.5 90/08/01 13:32:25 pb Exp $
.\"
.\" $Log:	install.ms,v $
.\" Revision 5.5  90/08/01  13:32:25  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:38:22  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:29  jpo
\" Initial revision
\" 
.\"
.NH
Installation of the Software
.LP
This should be achieved the first time by the command:
.DS
make fullinstall
.DE
which does a "make dirs" to create the necessary directories,
"make install" to install the files,
and "make keyfile" to generate the secret key for password encryption.
On subsequent installations, only the "make install" need be run.

Note that you will most likely need to be the super-user to complete
this step as it copies the software to the normal system directories.
It is probably worth running make with the `-n' flag at this point
just to be sure it isn't going to do something unexpected.
.LP
Some assistance in checking and setting the modes of files may be achied by
invoking make with the "showmodes" and "setmodes" keys.
See "make help" for the full set of meta keys for make.
