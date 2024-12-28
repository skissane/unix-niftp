# where THIS site's files are ...
SPOOL	= /usr/spool/niftp
LBINDIR	= /usr/lib/niftp
LIB	= $(LBINDIR)
UBINDIR	= /usr/bin
MAKE	= make
RANLIB	= /usr/bin/ranlib
PROFILE	=  
CFLAGS  = -O $I   $(PROFILE)
STRIP	=  
LDFLAGS	= $(STRIP) $(PROFILE)  
MAILER	= progmail
NDIRLIB	= 
LIBM	=   libmail.a
SUNBIN	= psun
CHUG	= /etc/chown
CHMOD	= /bin/chmod
CHGRP	= /bin/chgrp
CHOWN	= /etc/chown
GRP	= niftp
TAILOR	= /etc/niftptailor
NRSDBMD	= /usr/spool/niftp
NRSDBMB	= $(NRSDBMD)/nrsdbm
NRSDBM	= $(NRSDBMB).dir $(NRSDBMB).pag
SPOOLM	=   $(SPOOL)/mail
SPOOLN	=   $(SPOOL)/news
SPOOLNET= $(SPOOL)/qp $(SPOOL)/qj $(OTHERNET)
MKDIR775= $(LIB) $(SPOOL) $(SPOOL)/logs
MKDIR770= $(LIB)/key $(SPOOL)/dockets $(SPOOLNET) $(SPOOLM) $(SPOOLN)
UAIEFS	= UAIEF

SUNLIB	=	.
SENDLIB	=  	sendmail
LIBMAILER= 	.

TARVER	= # v

#- primary make options are:
#- all           do a standard make
#- fullinstall   do a "make mkdirs install keyfile patch"
#-
#- if anything unusual happens, the sub make options are:
#- help          list the make options (i.e. this)
#- patch         run dbpatch (add local address entries)
#- s{et,how}mode set or show the mode to critical files
#- dirs mkdirs   make all the necc directories
#- install       do a std "make install"
#- keyfile       create the master password file
#- core-tar      make a tar file of the core stuff
#- clean         do a std "make clean" on sub dirs
#- lint          do a std "make lint" on sub dirs


LIBG	= libgen.a
LIBP	= libp.a $(LIBM) $(LIBG)
PROC	= $(SUNBIN)
# install these so that they can be run by anyone
EXECIN	= $(LBINDIR)/lookdbm $(UBINDIR)/ftshow $(UBINDIR)/lookdbm \
	  $(LBINDIR)/dersplit
# install these priv programmes (setuid or setgid)
SETIN	= $(UBINDIR)/cpf $(UBINDIR)/ftpq $(UBINDIR)/ftprm $(UBINDIR)/qft
# This should be SETUID ROOT, execute access to group, NOT other
SETRIN	= $(LBINDIR)/killspool
# install these so that root can run them
ROOTIN	= $(LBINDIR)/ftpkey $(LBINDIR)/setup $(LBINDIR)/dbpatch \
	  $(LBINDIR)/ftpspool $(LBINDIR)/fpc

all	= std $(PROC)
std	= fpc ftprm cpf ftpspool ftpkey ftpq setup killspool \
	  dbpatch lookdbm ftshow 

all:    .README.stamp $(all)
std:	$(std)

help:	ALWAYS
	@-sed -e '/^#- /s///p' -e d makefile

core-tar: core-files
	tar c$(TARVER)fb core-niftp.tar 16 `cat core-files`

dirs mkdirs:	$(MKDIR775) $(MKDIR770)

niftptailor:
	@echo edit one of the samples/niftptailor.*
	@echo files to create an niftptailor
	@false

keyfile:	$(LIB)/key/keyfile
$(LIB)/key/keyfile:
	$(LIB)/ftpkey

README:	.README.stamp
.README.stamp: README.sunlink.sun3
	@sh README.sunlink.sun3

patch: .UAIEF.stamp
.UAIEF.stamp: $(UAIEFS) $(NRSDBMD) $(NRSDBM)
	for x in $(UAIEFS); do $(LBINDIR)/dbpatch < $$x; done
	touch $(LOCAL)/.UAIEF.stamp
$(NRSDBM):
	touch $@

$(MKDIR775):
	mkdir $@
	$(CHMOD) 775 $@
	$(CHGRP) $(GRP) $@
$(MKDIR770):
	mkdir $@
	$(CHMOD) 770 $@
	$(CHGRP) $(GRP) $@


# The main bits is the P process ...

# SOCKETS/SUNlink (sun)
sun sunlink: $(SUNBIN)

psun:  ftp.o $(LIBP) libnet.a $(NDIRLIB)
	cc -o psun $(LDFLAGS) ftp.o adr_trans.o $(LIBP) libnet.a $(LIBP)

# other misc programmes ...
fpc:	fpc.o $(LIBG) $(NDIRLIB)
	cc -o fpc $(LDFLAGS) fpc.o $(LIBG) $(NDIRLIB)

ftprm:  ftprm.o $(LIBG) $(NDIRLIB)
	cc -o ftprm $(LDFLAGS) ftprm.o $(LIBG) $(NDIRLIB)

cpf:	cpf.o $(LIBG) $(NDIRLIB)
	cc -o cpf $(LDFLAGS) cpf.o $(LIBG) $(NDIRLIB)

ftpq:   ftpq.o $(LIBG) $(NDIRLIB)
	cc -o ftpq $(LDFLAGS) ftpq.o $(LIBG) $(NDIRLIB)

ftpspool:  ftpspool.o $(LIBG) $(NDIRLIB)
	cc -o ftpspool $(LDFLAGS) ftpspool.o $(LIBG) $(NDIRLIB)

ftpkey: ftpkey.o $(LIBG) $(NDIRLIB)
	cc -o ftpkey $(LDFLAGS) ftpkey.o $(LIBG) $(NDIRLIB)

killspool:  killspool.o $(LIBG)
	cc -o killspool $(LDFLAGS) killspool.o $(LIBG)

setup:  setup.o $(LIBG) $(NDIRLIB)
	cc -o setup $(LDFLAGS) setup.o $(LIBG) $(NDIRLIB)

dbpatch: dbpatch.o $(LIBG) $(NDIRLIB)
	cc -o dbpatch $(LDFLAGS) dbpatch.o $(LIBG) $(NDIRLIB)

lookdbm: lookdbm.o $(LIBG) $(NDIRLIB)
	cc -o lookdbm $(LDFLAGS) lookdbm.o $(LIBG) $(NDIRLIB)

ftshow: ftshow.o $(LIBG) $(NDIRLIB)
	cc -o ftshow $(LDFLAGS) ftshow.o $(LIBG) $(LIBQ) $(NDIRLIB)

clean:
	-rm -f $(all) *.old

fullinstall: mkdirs install keyfile patch
install: inst-cpf inst-ftpq inst-ftprm inst-fpc inst-ftpspool inst-ftpkey \
	 inst-setup inst-killspool inst-dbpatch inst-lookdbm inst-ftshow \
	 inst-proc inst-niftptailor
inst-cpf:		$(UBINDIR)/cpf
inst-dbpatch:		$(LBINDIR)/dbpatch
inst-fpc:		$(LBINDIR)/fpc
inst-ftpkey:		$(LBINDIR)/ftpkey
inst-ftpq:		$(UBINDIR)/ftpq
inst-ftprm:		$(UBINDIR)/ftprm
inst-ftpspool:		$(LBINDIR)/ftpspool
inst-killspool:		$(LBINDIR)/killspool
inst-ftshow:		$(UBINDIR)/ftshow
inst-lookdbm:		$(UBINDIR)/lookdbm	# was $(LBINDIR)/lookdbm
inst-setup:		$(LBINDIR)/setup

$(LBINDIR)/dbpatch:	dbpatch
$(LBINDIR)/fpc:		fpc
$(LBINDIR)/ftpkey:	ftpkey
$(LBINDIR)/ftpspool:	ftpspool
$(LBINDIR)/killspool:	killspool
$(LBINDIR)/lookdbm:	lookdbm
$(LBINDIR)/setup:	setup
$(UBINDIR)/cpf:		cpf
$(UBINDIR)/ftpq:	ftpq
$(UBINDIR)/ftprm:	ftprm
$(UBINDIR)/ftshow:	ftshow
$(UBINDIR)/lookdbm:	lookdbm

$(SETIN):
	cp `expr $@ : '.*/\(.*\)'` $@
	-$(CHUG) root $@
	$(CHGRP) $(GRP) $@
	$(CHMOD) 2111 $@
$(SETRIN):
	cp `expr $@ : '.*/\(.*\)'` $@
	-$(CHUG) root $@
	$(CHGRP) $(GRP) $@
	$(CHMOD) 4110 $@
$(ROOTIN):
	cp `expr $@ : '.*/\(.*\)'` $@
	-$(CHUG) root $@
	$(CHGRP) $(GRP) $@
	$(CHMOD) 2110 $@
$(EXECIN):
	cp `expr $@ : '.*/\(.*\)'` $@
	-$(CHUG) root $@
	$(CHGRP) $(GRP) $@
	$(CHMOD) 111 $@
inst-niftptailor:	$(TAILOR)
$(TAILOR):	niftptailor
	cp niftptailor $(TAILOR)
inst-proc: $(PROC)
	cp $(PROC) $(LBINDIR)
	cd $(LBINDIR); $(CHGRP) $(GRP) $(PROC); $(CHMOD) 2110 $(PROC)
tidy:
	@echo no tidy to do ...

clobber: tidy
	rm -f $(all)

#	anything else ??

setmode:
	-for x in $(SETIN); \
	 do $(CHUG) root $$x; $(CHGRP) $(GRP) $$x; $(CHMOD) 2111 $$x; done
	-for x in $(SETRIN); \
	 do $(CHUG) root $$x; $(CHGRP) $(GRP) $$x; $(CHMOD) 4111 $$x; done
	-for x in $(ROOTIN); \
	 do $(CHUG) root $$x; $(CHGRP) $(GRP) $$x; $(CHMOD) 2110 $$x; done
	-for x in $(EXECIN); \
	 do $(CHUG) root $$x; $(CHGRP) $(GRP) $$x; $(CHMOD) 0111 $$x; done
	-for x in $(PROC); \
	 do $(CHUG) root $$x; $(CHGRP) $(GRP) $$x; $(CHMOD) 2110 $$x; done
	-for x in $(MKDIR775); \
	 do $(CHUG) root $$x; $(CHGRP) $(GRP) $$x; $(CHMOD) 775 $$x; done
	-for x in $(MKDIR770); \
	 do $(CHUG) root $$x; $(CHGRP) $(GRP) $$x; $(CHMOD) 770 $$x; done

showmode:
	@ls -lgd $(SPOOL1) $(MKDIR775) $(MKDIR770) $(SETIN) $(SETRIN) $(ROOTIN) $(EXERCIN) $(PROC)
