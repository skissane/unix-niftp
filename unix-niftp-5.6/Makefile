# RCSid[] = "@(#)$Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/RCS/Makefile,v 5.5 90/08/01 13:28:07 pb Exp $"
#
# This is just enough to generate the real makefile
#
# need this for HP-UX
SHELL	= /bin/sh
#
MAKE	= make
SPEED	= 
OPT	= $(SPEED)

configure:	Configure makefile.SH opts.SH gen.conf.SH
	sh Configure $(OPT)
	@echo " "
	@if [ -r makefile ] ; \
	then echo now doing the real make ...; echo " "; \
	     $(MAKE) $(MFLAGS) all keyfile; \
	else echo now do the real make, including keyfile ..... ; \
	fi
