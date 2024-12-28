# /etc/niftptailor
#
# small JANET only site, without news, all compiled in values correct, ONLY PP

# NRS configuration file
TABLE   /usr/lib/niftp/nrsdbm
DQUEUE  /usr/spool/niftp          # default queue
# the directory containing the default binaries
BINDIR  /usr/lib/niftp

# the default logfile directory
LOGDIR  /usr/spool/niftp/logs

QADDRTYPE  ftp,ftp, ftp.mail,pp, ftp.pp,pp, ftp.news,news

# known about queues (support multiple P processes for mail and news)
QUEUE   qj      level=107,prog=psun


# various configuration details for each network
NET janet       queue=qj,address="000008010080/%E%X%D/%T"

DOMAIN  "uk.ac.cam.cl","uk.ac.cambridge.computer-lab","uk.ac.cam","uk.ac.cambridge","uk.ac","uk","uk.co" # known domains

# Separate listeners for FTP and PP
# This is useful on machines with super servers (e.g. ybtsd on SUNs)
# as it costs nothing extra, keeps the logs separate, and allows control
# over transfers.
LISTEN lf	address="FTP",level=107,prog=qsun,channel=janet
LISTEN lm	address="FTP.MAIL",level=107,prog=qsun,channel=janet

# However, on machines where there are separate listener processes and
# too many would impact the sysyem performance, this can be simplified to
LISTEN lj       address="FTP",level=107,prog=qsun,channel=janet
