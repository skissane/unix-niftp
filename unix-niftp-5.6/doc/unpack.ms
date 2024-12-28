.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/unpack.ms,v 5.5 90/08/01 13:32:56 pb Exp $
.\"
.\" $Log:	unpack.ms,v $
.\" Revision 5.5  90/08/01  13:32:56  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:40:14  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:54  jpo
\" Initial revision
\" 
.\"
.NH
Unpacking the Distribution.
.LP
There are a number of ways in which you may receive the distribution.
These are generally either by tape, NIFTP or as several mail messages.
Essentially though, they are basically all the same in that you should
end up with a tar image to extract. Decide which of the methods listed
below is the one that reflects your situation and follow the instructions.
.br
Each way will eventally leave you with a tar file.
Create a directory for the image to reside in - this can be anywhere
in the file system, but is usually somewhere like /usr/src/unix-niftp
or /usr/src/local/unix-niftp.
.DS
mkdir /usr/src/local/unix-niftp
cd /usr/src/local/unix-niftp
.DE
.NH 2
Tar Tape
.LP
To unpack a tar image sent on a tape, read in the tape the directory with
the appropriate tar command.
.DS
tar xv
.DE
You should now have the distribution unpacked and ready to compile.
.NH 2
NIFTP
.LP
If you have received the tar image via
NIFTP you will most likely find that it has been compressed to reduce
space and transmission overhead with the
.I compress (1)
program. The compress program is widely available and is distributed
as standard with most versions of
.UX
now.
The version currently in use is version 4.0, if you don't have this
version you should get it.
.LP
Take the ftp file and uncompress it if necessary, and then extract the
contents. This can be done as one operation, which is useful if disk space
is tight.
.DS
uncompress < unix.niftp.tar.Z | tar xvf -
.DE
This process should contain no errors. If it does, then it is either
the case that you have a corrupt distribution (this happens frequently
when copying binary files using old niftp code) or you don't have
permissions to write in the directory you have created!
.NH 2
Mail Distribution
.LP
This is basically the same as the NIFTP version, so only the changes are
noted here.
The distribution is almost certain to be compressed and then fed
through a binary to ascii translation program. The two most like
translations are uuencode/uudecode and atob/btoa. The distribution will
be also usually be in separate messages, you must first edit these
together remvoing extraneous headers and footers. Make sure you put
them together in the right order too!
Once you have done all this you should be left with one file. You now
need to convert it to the binary form. This is most usually uudecode
but try atob if you have it and uudecode fails.
.DS
uudecode < file
atob < file > unix-niftp.tar.Z
.DE
From this stage on it is the same as the NIFTP case.
