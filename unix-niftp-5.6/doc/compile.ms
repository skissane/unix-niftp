.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/compile.ms,v 5.5 90/08/01 13:32:23 pb Exp $
.\"
.\" $Log:	compile.ms,v $
.\" Revision 5.5  90/08/01  13:32:23  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:38:20  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:28  jpo
\" Initial revision
\" 
.\"
.NH
Compile time tailoring
.LP
This process should be fairly painless. It consists of running the
script
.I Configure
script and answering the questions it asks. The configure script
attempts to discover as much as it can for itself, but will in some
cases ask for confirmation. In other cases it will ask direct
questions about where to put binaries and so on.
.LP
Selecting options at compilation may not be easy, if you have
installed any PD software you will be used to questions about
compiler options and so on - but the questions that you may find
hard are the ones about network options.  There is a simple rule
here, if you have not heard of it - don't select it.  You may
cause compilation to fail when it does not find the code.
.LP
If you are using sendmail, you probably want the compile option
.DS
-DMAILOWNSMAIL
.DE
which causes all qmail in the niftp queues to be owned by the mail user,
rather than the sender of the mail.
.LP
When it has finished, Configure will update several files with its
results and you are then ready to compile. However, it is worth
checking certain files to see if the values look reasonable. A list of
the current defines and is given in Appendix 1.
.LP
Now, to compile, simply type make and watch the files fly. If it
throws up errors, you may have specified something wrong in the
Configure or else you may have a weird system.

