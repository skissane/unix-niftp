.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/nrs.ms,v 5.5 90/08/01 13:32:42 pb Exp $
.\"
.\" $Log:	nrs.ms,v $
.\" Revision 5.5  90/08/01  13:32:42  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:39:17  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:36  jpo
\" Initial revision
\" 
.\"
.NH
Use with the NRS database.
.LP
The \*(uN package relies fairly heavily on the NRS database. This
needs to be converted into a 
.I dbm (3)
database before it is usable. This is best done in the following way.
.br
If the NRS tables are to be used, the bulk of the dbm file is generated
using the dbencode command.
Use c-nrs (contact pb@uk.ac.cam.cl for details) to generate this.
.br
In any case, it will be necessary to add some entries not in the NRS.
If dbencode is not use, just "touch" the .pag and .dir files.
Having generated a suitable UAIEF file (see samples/), dbpatch will make
the necessary changes, e.g.
.DS
nrs dbm1.cf
dbencode < dbm
dbpatch < UAIEF
.DE
