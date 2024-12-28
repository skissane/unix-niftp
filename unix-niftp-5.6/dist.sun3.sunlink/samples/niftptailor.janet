FTPUSER		root
MAILUSER	root
NEWSUSER	root
MAILPROG	"/usr/lib/sendmail"
DOCKETDIR	"/usr/spool/niftp/dockets"
SPOOLER		"/usr/spool/niftp/ftpspooler"

# NRS configuration file
TABLE   /usr/spool/niftp/nrsdbm

DQUEUE  /usr/spool/niftp          # default queue

# the directory containing the default binaries
BINDIR  /usr/lib/niftp

# the default logfile directory
LOGDIR  /usr/spool/niftp/logs

SETUPPROG  /usr/lib/niftp/setup
KEYFILE    /usr/lib/niftp/key/keyfile
SECUREDIRS /usr/lib/niftp/securedirs

DOMAIN  "uk.ac","uk","uk.co" # known domains

# known about queues
QUEUE   qj      level=127,prog=psun

# various configuration details for each network
NET janet	queue=qj,address="<put your DTE here>/%E%X%D/%T"

LISTEN lj	level=111
