case $CONFIG in
    '') . config.sh ;;
esac
outputfile=Makefile.com
rm -f $outputfile.$$
echo "Creating $outputfile (with variable substitutions)"
if cat >$outputfile.$$ <<!GROK!THIS!
# unix-niftp Makefile
# RCSid="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/RCS/makefile.SH,v 5.6.1.10 1993/05/17 13:17:01 pb Exp $"
RCSsymbNAME=May1793FixVMSStreamCRLF
RCSmessage =Re-do Format negotiation to avoid 0x80 VMS xfers with CRLF
RCSbase =5.6.1
RCSVERS	= -r\$(RCSbase)
RCSDIFFFLAGS = -c \$(RCSVERS)
COFLAGS = -q
CIFLAGS = -k    # or -r\$(RCSver)
# overall Makefile
#
# need this for HP-UX
SHELL	= /bin/sh
#
# where THIS site's files are ...
LOCAL	= $local
SPOOL	= $spool
MAILSP	= $hashmailer $mailspooldir
NEWSSP	= $hashnews $newsspooldir
MAILMOD	= $mailmode
NEWSMOD	= $newsmode
LBINDIR	= $privatebin
LIB	= \$(LBINDIR)
UBINDIR	= $publicbin
MAKE	= make
ENVPPLD	= $envppld
LIBPP	= $pplib
LIBDES	= $deslib $krblib
RANLIB	= $ranlib
H	= \$p/h
I	= -I\$H -I.
PROFILE	= $profile
CFLAGS  = -O \$I $cflags $(PROFILE) $(LCFLAGS) $krbcflags
STRIP	= $strip
LDFLAGS	= $(STRIP) $(PROFILE) $ldflags
MAILER	= $maildir
RMTDBM	= $hashrmtdbm in.rmtdbm
RMTDBMD	= $hashrmtdbmd $rmtdbmd
NDIRLIB	= $ndirlib
LIBM	= $hashmailer \$p/lib/$(MAILER)/libmail.a
DEXBIN	= $hashddex pdex  qdex
INEBIN	= $hashdinet pinet qinet
IPCBIN	= $hashdipcs pipcs qipcs
YORBIN	= $hashdyork pyork qyork
X25BBIN	= $hashdx25b px25b qx25b
UBCBIN	= $hashdubc pubc  qubc
SUNBIN	= $hashdsun psun  qsun # qsun7 qsun6 psun7 psun6
SYMBIN	= $hashdseq pseq  qseq
ALLBIN	= \$(DEXBIN) \$(INEBIN) \$(IPCBIN) \$(YORBIN) \$(X25BBIN) \\
	  \$(UBCBIN) \$(SUNBIN) \$(SYMBIN)
CHUG	= $chug
CHMOD	= $chmod
CHGRP	= $chgrp
CHOWN	= $chown
GRP	= $grp
CP	= $cp

# Fix for HPUX:
SHELL	= /bin/sh

!GROK!THIS!
then if cmp $outputfile $outputfile.$$ > /dev/null 2>& 1
	then
		rm -f $outputfile.$$
		$echoq "	" $outputfile not affected
	elif mv $outputfile.$$ $outputfile
	then
		echo "	" $outputfile updated
	else
		echo "+++	" $outputfile left in $outputfile.$$
	fi
else
	echo "	" Failed to create $outputfile.$$
fi

outputfile=makefile
rm -f $outputfile.$$
echo "Creating $outputfile (with variable substitutions)"
if cat Makefile.com - >$outputfile.$$ <<!GROK!THIS!
H	= \$\$p/h
I	= -I\$H -I.
LIBM	= $hashmailer \$\$p/lib/$(MAILER)/libmail.a

# Add other bits as needed ....
MAKEPARAMS = SHELL="\$(SHELL)" \\
		RANLIB="\$(RANLIB)" MAKE="\$(MAKE)" SPOOL="\$(SPOOL)" \\
		MAILSP="\$(MAILSP)" NEWSSP="\$(NEWSSP)" \\
		MAILMOD="\$(MAILMOD)" NEWSMOD="\$(NEWSMOD)" \\
		MAILER="\$(MAILER)" NDIRLIB="\$(NDIRLIB)" \\
		LBINDIR="\$(LBINDIR)" UBINDIR="\$(UBINDIR)" \\
		RMTDBMD="\$(RMTDBMD)" RMTDBM="\$(RMTDBM)" \\
		H="\$H" CFLAGS="\$(CFLAGS)" LDFLAGS="\$(LDFLAGS)" \\
		LIBM="\$(LIBM)" P="\$\$p" LIBPP="\$(LIBPP)" \\
		ENVPPLD="\$(ENVPPLD\)" LIBDES="\$(LIBDES)" \\
		DEXBIN="$(DEXBIN)" INEBIN="$(INEBIN)" IPCBIN="$(IPCBIN)" \\
		X25BBIN="$(X25BBIN)" YORBIN="$(YORBIN)" UBCBIN="$(UBCBIN)" \\
		SUNBIN="$(SUNBIN)" SYMBIN="$(SYMBIN)" \\
		NIFTPINTERFACE="$(NIFTPINTERFACE)" \\
		CHGRP="$(CHGRP)" CHMOD="$(CHMOD)" CHOWN="$(CHOWN)" \\
		CHUG="$(CHUG)" GRP="$(GRP)" CP="$(CP)"
# Used locally only ...
#	mkdir these ...
SPOOLM	= $hashmailer \$(SPOOL)/mail
SPOOLN	= $hashnews \$(SPOOL)/news
SPOOLNET= \$(SPOOL)/qe \$(SPOOL)/qj \$(OTHERNET)
MKDIR775= \$(LIB) \$(LOCAL) \$(SPOOL) \$(SPOOL)/logs
MKDIR770= \$(LIB)/key \$(SPOOL)/dockets \$(SPOOLNET) \$(SPOOLM) \$(SPOOLN) \\
	  \$(MAILSP) \$(NEWSSP) 
UAIETH	= \$(LOCAL)/UAIEF.ether
UAIEFS	= \$(LOCAL)/UAIEF $hashinet \$(UAIETH)
CONFIGS	= makefile.SH opts.SH gen.conf.SH mk.sun.link.SH

DEXLIB	= $hashddex	lib/dexpand
INELIB	= $hashdinet	lib/inet
IPCLIB	= $hashdipcs	lib/ipcs
YORLIB	= $hashdyork	lib/yorkbox
X25BLIB	= $hashdx25b	lib/x25b
UBCLIB	= $hashdubc	lib/ubc
SUNLIB	= $hashdsun	lib/sun
SYMLIB	= $hashdseq	lib/seq
DIRLIB	= $hashndir	ndir
SENDLIB	= $hashsendmail	sendmail
NEWSLIB	= $hashnews	# news
LIBMAILER=$hashmailer	lib/\$(MAILER)
INSTXT	= doc/installation.txt

TARVER	= # v

# the directories that are called to make the system
MAKEDIR	= \$(DIRLIB) lib/gen lib/pqproc \$(LIBMAILER) \\
	  \$(DEXLIB) \$(INELIB) \$(IPCLIB) \$(YORLIB) \$(X25BLIB) \\
	  \$(UBCLIB) \$(SUNLIB) \$(SYMLIB) bin \$(SENDLIB) \$(NEWSLIB)
ALLDIRS	= bin ndir news sendmail lib/mmdf lib/progmail lib/pqproc lib/gen \\
	  lib/dexpand lib/inet lib/ipcs lib/sun lib/ubc lib/x25b lib/yorkbox \\
	  lib/seq doc

# the source directories (i.e. make shar)
SRCDIR	= \$(MAKEDIR) h
# the directories for which the 'general' make commands are available,
# e.g. clean, install lint, ...
GENDIR	= \$(MAKEDIR)
RCS	= \`cat rcs-files\`
BINO	= cpf.o dbencode.o dbpatch.o fixtime.o fpc.o ftp.o \\
	  ftpkey.o ftpq.o ftpreq.o ftprm.o ftpspool.o ftq.o ftshow.o \\
	  killspool.o lookdbm.o qft.o setup.o
# dbextract.o 
BINS	= cpf dbencode dbpatch fixtime ftp ftpkey ftpq ftpreq \\
	  ftprm ftpspool ftq ftshow killspool lookdbm niftp \$(ALLBIN) qft \\
	  setup greyout xgreyout
# dbextract 
#- make options are:
#- all           do a standard make (configure & sub dirs)
#- fullinstall	 do a "make mkdirs install keyfile"
#- install       do a std "make install" on sub dirs
#- dirs mkdirs   make all the necc directories
#- keyfile       create the master password file
#- localfiles    install the local files (from the directory local)
#- patch         run dbpatch (add local address entries)
#- help          list the make options (i.e. this)
#- s{et,how}mode set or show the mode to critical files
#- configure     force reconfiguration
#-
#- Not of general interset:
#- bin.o         make the bin/*.o
#- core-tar      make a tar file of the core stuff
#- core-cpio     make a cpio file of the core stuff
#- full-tar      make a tar file of everything
#- clean         do a std "make clean" on sub dirs
#- lint          do a std "make lint" on sub dirs
#- ci co rcsdiff use RCS to maintain previous info

all: config.sh \${MAKEDIR}

.PRECIOUS: config.sh

\${ALLDIRS}: ALWAYS
	@echo "cd \$@; \$(MAKE) \$(MFLAGS)"
	@p=\`pwd\`; cd \$@; \$(MAKE) -f Makefile.real \$(MFLAGS) \$(MAKEPARAMS)

# Allow making of individual bin files (e.g. after new library)
\${BINS} \${BINO}: ALWAYS
	@echo "cd bin; \$(MAKE) \$(MFLAGS) \$@"
	@p=\`pwd\`; cd bin; \$(MAKE) -f Makefile.real \$(MFLAGS) \$(MAKEPARAMS) \$@
bin.o:	ALWAYS
	@echo "cd bin; \$(MAKE) \$(MFLAGS) \${BINO}"
	@p=\`pwd\`; cd bin; \$(MAKE) -f Makefile.real \$(MFLAGS) \$(MAKEPARAMS) \${BINO}

lib/pqproc/libppp.a: ALWAYS
	@echo "cd lib/pqproc; \$(MAKE) \$(MFLAGS) libppp.a"
	@p=\`pwd\`; cd lib/pqproc; \$(MAKE) -f Makefile.real \$(MFLAGS) \$(MAKEPARAMS) libppp.a

# Allow making of individual bin files (e.g. after new library)
\${INSTXT}: doc

fullinstall: mkdirs install keyfile
clean install lint: ALWAYS
	-p=\`pwd\`; \\
	  for d in \${GENDIR}; \\
	  do (cd \$\$d; echo "cd \$\$d; \$(MAKE) \$@"; \\
		 \$(MAKE) -f Makefile.real \$(MFLAGS) \$(MAKEPARAMS) \$@); \\
	  done

setmode: setaccess setaccess-root
showmode: showaccess showaccess-root

setaccess showaccess: ALWAYS
	@cd bin; \$(MAKE) -f Makefile.real \$(MFLAGS) \$(MAKEPARAMS) \$@

ALWAYS:

configure:	Configure \$(CONFIGS)
	sh Configure quick

config.sh:	Configure \$(CONFIGS)
	sh Configure quick
	@echo " "
	@-if [ -r makefile ]; \\
	then echo now doing the real make .... ; echo " "; \\
	     \$(MAKE) \$(MFLAGS); \\
	else echo now do the real make ..... ; \\
	fi
	kill \$\$\$\$

help:	ALWAYS
	@-sed -n -e '/^#- /s///p' makefile

cx      : ALWAYS	# private HACK !!!!
	@echo \$(CXCMD) \$(CXFLAGS)   ... well almost ...
	@-for target in \$(RCS) ; do \\
	 (if dir="\`expr \$\$target : '\(.*\)/[^/]*'\`"; then cd \$\$dir;fi;\\
          file="\`basename \$\$target\`"; \\
	  $(CXCMD) \$(CXFLAGS) \$\$file; \\
	 ) \\
	done

co      : ALWAYS
	@echo co \$(COFLAGS) \$(RCS) ... well almost ...
	@-for target in \$(RCS) ; do \\
	 (if dir="\`expr \$\$target : '\(.*\)/[^/]*'\`"; then cd \$\$dir;fi;\\
          file="\`basename \$\$target\`"; \\
	  co \$(COFLAGS) \$\$file; \\
	 ) \\
	done

ci      : ALWAYS
	@echo ci -r$(RCSbase) -m\\"Distribution of \$(RCSsymbNAME): \\
	 \$(RCSmessage)\\" \$(CIFLAGS) -N\\"\$(RCSsymbNAME)\\" \\
	 ... well almost ...
	@-for target in \$(RCS) ; do \\
	 (rc=true; \\
	  if dir="\`expr \$\$target : '\(.*\)/[^/]*'\`"; then cd \$\$dir;fi;\\
          file="\`basename \$\$target\`"; \\
	  if [ -r "\$\$file" ]; \\
	  then  if co -p\$(RCSbase) -q "\$\$file" | cmp -s - "\$\$file"; \\
		then    rm -f "\$\$file"; \\
		else    rcs -l\$(RCSbase) "\$\$file"; \\
			ci -r\$(RCSbase) -m"Distribution of \$(RCSsymbNAME): \$(RCSmessage)" \$(CIFLAGS) -N"\$(RCSsymbNAME)" "\$\$file"; \\
			rc=false; \\
		fi; \\
	  fi; \\
	  if \$\$rc; \\
	  then  eval \`rlog -h \$\$file | awk '{ if (\$\$1 == "head:") print "vers=\\"" \$\$2 "\\""}'\`; \\
		rcs -N\$(RCSsymbNAME):\$\$vers -sRel "\$\$file"; \\
	  fi; \\
         ); \\
	done

rcsdiff : ALWAYS
	@echo rcsdiff $(RCSDIFFFLAGS) \$(RCS) ... well almost ...
	@-for target in version.h \$(RCS) ; do \\
	 (if dir="\`expr \$\$target : '\(.*\)/[^/]*'\`"; then cd \$\$dir;fi;\\
          file="\`basename \$\$target\`"; \\
	  echo Index: \$\$target; \\
	  rcsdiff \$(RCSDIFFFLAGS) \$\$file; \\
	 ) \\
	done

core-tar: core-files $(INSTXT)
	cd ..; tar c\$(TARVER)fb core-niftp.tar 16 \`cat core-files\`

core-cpio: core-files $(INSTXT)
	cd ..; cpio -oc\$(TARVER) > core-niftp.cpio < core-files

full-tar: full-files
	cd ..; tar cvfb full-niftp.tar 16 \`cat niftp-files\`

full-cpio: full-files
	cd ..; cpio -ocv > full-niftp.cpio < full-files

#does not work at present
shardist:
	for dir in \${SRCDIR} ; do \\
	  (echo \$\${dir}; cd \$\${dir}; \$(MAKE) \$(MFLAGS) shardist); done

dirs mkdirs:	\$(MKDIR775) \$(MKDIR770)

keyfile:	mkdirs \$(LIB)/key/keyfile
\$(LIB)/key/keyfile:
	\$(LIB)/ftpkey

localfiles:	/etc/niftptailor \$(LIB)/securedirs \$(LIB)/UAIEF
/etc/niftptailor:	\$(LOCAL)/niftptailor
	$(CP) \$(LOCAL)/niftptailor	/etc
\$(LIB)/securedirs:	\$(LOCAL)/securedirs
	$(CP) \$(LOCAL)/securedirs	\$(LIB)
\$(LIB)/UAIEF:	\$(LOCAL)/UAIEF
	$(CP) \$(LOCAL)/UAIEF	\$(LIB)

patch: \$(LOCAL)/.UAIEF.stamp
\$(LOCAL)/.UAIEF.stamp: \$(UAIEFS)
	for x in \$(UAIEFS); do \$(LBINDIR)/dbpatch < \$\$x; done
	touch \$(LOCAL)/.UAIEF.stamp

\$(MKDIR775):
	mkdir \$@
	$(CHMOD) 775 \$@
	$(CHGRP) $(GRP) \$@
\$(MKDIR770):
	mkdir \$@
	$(CHMOD) 770 \$@
	$(CHGRP) $(GRP) \$@

setaccess-root:
	-for x in \$(MKDIR775); \\
	 do \$(CHUG) root \$\$x; \$(CHGRP) \$(GRP) \$\$x; \$(CHMOD) 775 \$\$x; done
	-for x in \$(MKDIR770); \\
	 do \$(CHUG) root \$\$x; \$(CHGRP) \$(GRP) \$\$x; \$(CHMOD) 770 \$\$x; done

showaccess-root:
	@ls -lgd \$(SPOOL1) \$(MKDIR775) \$(MKDIR770)
!GROK!THIS!
then if cmp $outputfile $outputfile.$$ > /dev/null 2>& 1
	then
		rm -f $outputfile.$$
		$echoq "	" $outputfile not affected
	elif mv $outputfile.$$ $outputfile
	then
		echo "	" $outputfile updated
	else
		echo "+++	" $outputfile left in $outputfile.$$
	fi
else
	echo "	" Failed to create $outputfile.$$
fi
