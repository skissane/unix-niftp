# /etc/niftptailor
#
# All singing, all dancing example for a JANET, PSS and IPSS machine
# acting as a major MAIL and NEWS relay

# NRS configuration file
TABLE   /usr/lib/niftp/nrsdbm
DQUEUE  /usr/spool/niftp          # default queue
# the directory containing the default binaries
BINDIR  /usr/lib/niftp

# the default logfile directory
LOGDIR  /usr/spool/niftp/logs
NEWSPROC "/usr/lib/niftp/lognews %s"
NEWSUSER news

# Default mappings for YBTS -> actual function
#- QADDRTYPE  ftp,ftp, ftp.mail,pp, ftp.pp,pp, ftp.news,news
QADDRTYPE  ftp,ftp, ftp.mail,pp, ftp.pp,pp, ftp.news,news

# known about queues (support multiple P processes for mail and news)
QUEUE   qj      level=107,prog=psun
QUEUE   qj2     level=107,prog=psun,master=qj
QUEUE   qj3     level=107,prog=psun,master=qj
QUEUE   qn      level=107,prog=psun
QUEUE   qn2     level=107,prog=psun,master=qn
QUEUE   qn3     level=107,prog=psun,master=qn
QUEUE   qn4     level=107,prog=psun,master=qn

QUEUE   qp      level=107,prog=psun
QUEUE   qp2     level=107,prog=psun,master=qp
QUEUE   qp3     level=107,prog=psun,master=qp
QUEUE   qo      level=107,prog=psun
QUEUE   qo2     level=107,prog=psun,master=qo
QUEUE   qo3     level=107,prog=psun,master=qo
QUEUE   qo4     level=107,prog=psun,master=qo

# various configuration details for each network
NET janet       queue=qj,address="000008010080/%E%X%D/%T"
NET jnews       queue=qn,address="000008010080/%E%X%D/%T"
NET pss         queue=qp,address="234222339399/%E%X%D/%T"
NET pnews       queue=qo,address="234222339399/%E%X%D/%T"

DOMAIN  "uk.ac.cam.cl","uk.ac.cambridge.computer-lab","uk.ac.cam","uk.ac.cambridge","uk.ac","uk","uk.co" # known domains

# Separate listeners for FTP, MAIL and NEWS on JANET, PSS and IPSS
# This is useful on machines with super servers (e.g. ybtsd on SUNs)
# as it costs nothing extra, keeps the logs separate, and allows control
# over transfers.
LISTEN ljf	address="FTP",level=107,prog=qsun,channel=janet
LISTEN ljm	address="FTP.MAIL",level=107,prog=qsun,channel=janet
LISTEN ljp	address="FTP.PP",level=107,prog=qsun,channel=janet
LISTEN ljn	address="FTP.NEWS",level=107,prog=qsun,channel=janet

LISTEN lpf	address="FTP",level=107,prog=qsun,channel=pss
LISTEN lpm	address="FTP.MAIL",level=107,prog=qsun,channel=pss
LISTEN lpp	address="FTP.PP",level=107,prog=qsun,channel=pss
LISTEN lpn	address="FTP.NEWS",level=107,prog=qsun,channel=pss

LISTEN lif	address="FTP",level=107,prog=qsun,channel=ipss
LISTEN lim	address="FTP.MAIL",level=107,prog=qsun,channel=ipss
LISTEN lip	address="FTP.PP",level=107,prog=qsun,channel=ipss
LISTEN lin	address="FTP.NEWS",level=107,prog=qsun,channel=ipss

#+ # However, on machines where there are separate listener processes and
#+ # too many would impact the sysyem performance, this can be simplified to
#+ LISTEN lj       address="FTP",level=107,prog=qsun,channel=janet
#+ LISTEN lp       address="FTP",level=107,prog=qsun,channel=pss
#+ LISTEN li       address="FTP",level=107,prog=qsun,channel=ipss

#+ # or if the channel info is not wanted, it can be reduced to:
#+ LISTEN lg       address="FTP",level=107,prog=qsun,channel=grey
