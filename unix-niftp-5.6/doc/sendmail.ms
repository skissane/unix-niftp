.\" $Header: /Nfs/heaton/glob/src/usr.lib/niftp/src/doc/sendmail.ms,v 5.5 90/08/01 13:32:52 pb Exp $
.\"
.\" $Log:	sendmail.ms,v $
.\" Revision 5.5  90/08/01  13:32:52  pb
.\" Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
.\" 
\" Revision 5.2  89/01/13  14:40:08  pb
\" Distribution of Jan89SuckMail: Support Sucking of mail
\" 
\" Revision 1.1  88/04/05  08:32:48  jpo
\" Initial revision
\" 
.\"
.NH 2
Configuring with Sendmail
.NH 3
Configuring \*(uN
.LP
If you use Sendmail as your mail handler, you must say so in the Configuration
phase of \*(uN. Most of the work will then be done for you automatically.
When running Sendmail it is normal for mail to be owned and handled by the
user
.B root.
.LP
Sendmail will be used for forwarding mail received from the network,
and also for notifying users if something goes wrong with a
file-transfer request.
The way sendmail is invoked and the parameters passed can be
configured in /etc/niftptailor, using the MAILPROG and MAILFMT lines.
The `mail user' can also be specified.
These items should default to appropriate values, but you may wish to
specify them anyway:
.DS
MAILUSER        root
MAILPROG        /usr/lib/sendmail
MAILFMT sendmail, "%s -t", "%s -ba -oiY -oMs%s", 7
.DE
which means ...
.NH 3
Configuring Sendmail
.LP
The real work comes when you start to configure Sendmail itself!
If you are thinking of sitting down to edit a sendmail.cf file,
.B STOP!
Get a copy of Jim Crammond's UK-sendmail-1.4a configuration compiler.
.NH 4
Getting UK-sendmail-1.4a
.LP
If you already have access to network mail, send a
message to info-server@uk.ac.ukc with the following contents:
.DS
request: sources
topic: uk-sendmail-1
topic: uk-sendmail-2
topic: uk-sendmail-3
topic: uk-sendmail-4
topic: uk-sendmail-5
topic: uk-sendmail-6
request: end
.DE
.LP
You will receive six messages in reply. Trim off the mail headers and feed
each message in turn to sh(1). The result will be a system for constructing
sendmail configurations.
.LP
If you have not got access to mail yet (which is likely, since this document
is telling you how to set it up!), you will have to get the package by
some other means.
.LP
Even if you are not installing \*(uN in the UK, the configuration package
is worth considering as it takes a lot of the hassle out of setting up
Sendmail. The rest of this section assumes that you will use UK-sendmail-1.4a.
.NH 4
Choosing a domain name for mail
.LP
The first thing to do is to choose a name for your mail `domain'.
This must follow certain rules: in particular, you will almost certainly
be part of some larger domain which already has a name.
UK academic organisations will have names starting `uk.ac.' (note that the
largest domain is given first here, which is the reverse of normal practise.
Only the UK suffers from this, but it is relevant to \*(uN, being a
package of UK origin).
UK commercial organisations have names starting `uk.co.'.
Similar domain names exist in other countries.
A new university at Tring in the UK might choose the name `uk.ac.tring'.
The UK's NRS allows each organisation to have
.I two
names: one long and one short. The short name must fit within 18 characters,
but there is no (specified) limit on long names. If at all possible, you
should choose a
.B single
name, and keep it short. Where your domain name is used by UK-sendmail, it
will usually be the short form.
.LP
One final word on choosing names: do not name your machines by their
CPU type. One day you might replace that VAX with a Cray running
Berkeley 4.7; you would not want to keep the same mail address on
.I that!
.br
Also avoid use of a final component which might be a top level domain.
Most new top level domains will be the two letter ISO values.
.LP
If part of your organisation already has an electronic mail address you
should discuss domain names with them and try to present a consistent view
to `the world outside'. This is made quite easy by UK-sendmail's
`Multihost configuration' option.
.NH 4
Making the first configuration
.LP
You will eventually need to make a configuration for each machine in
your organisation. Many of these will be identical, so for the moment
let us consider only the machine that will be running \*(uN. This will
be the main mail interface to the outside world. Let us suppose that
it is a Sun, called Suleika.
.LP
In the UK-sendmail directory, type
.B MAKE
(You may need to `chmod\ +x\ MAKE' first).
The first question it will ask is the hostname, which defaults to the
name of the machine you are using. This name is used as the name of a
subdirectory in which the configuration will be constructed.
.DS
Suleika% MAKE
Sendmail initial configuration program  UK-1.4

Enter hostname (Suleika): 
.DE
.LP
On this occasion we accept the default by hitting <return>.
When making configurations for other machines, the name of the
configuration can be given at this stage. (It does not
.I have
to be the name of a machine)
.LP
The next question asks for the sitename, which also defaults to the
machine name (obtained with
.I hostname(1)
).
At this stage it is best to give the significant part of your domain
name, so Tring University would give the `tring' part of
`uk.ac.tring':
.DS
Enter sitename (Suleika): tring
.DE
.LP
The sitename is used to identify data that is valid for all machines
on that site.
.LP
The next question concerns `multihost sites'. If you have (or expect
to have) more than one mail-handling computer, you should probably go
for the multihost configuration:
.DS
Is this a multihost site, where a collection of hosts
are made to look like one site? (y|n) y
.DE
.LP
The machine (or configuration) name will become part of the `complete
domain name'. Sendmail will use this name internally, but the
machine-name part will never appear in addresses sent off-site. 
Domain names are specified in `little-endian' order from now on:
.DS
Complete domain name: Suleika.tring.ac.uk

Host name = Suleika, Site name = tring,
Domain name = tring.ac.uk (multihost site)
confirm? (y|n) 
.DE
.LP
If all that is OK, answer `y' and carry on to select the mailers that
you will use. In this context, a mailer is a means of sending a
message to a remote machine, so it includes \*(uN, uucp, and such
like. Assuming that Tring has an Ethernet and wants to use \*(uN for
all off-campus mail, the dialogue would be something like this:
.DS
ethernet? y
Domain name on ethernet channel (tring.ac.uk): 
local uucp? n
uucp? n
janet? y
Domain name on janet channel (in 822 format) (tring.ac.uk): 
bitnet? n
csnet? n
 
        Are you using bsd4.3 or Sun 3.0 sendmail? (y|n) y
 
Making directory Suleika... 
creating rules... 
creating mailers... 
creating Makefile... 
creating Suleika.mc... 
Initial configuration complete.

You now have to make the tring.dom and Suleika.chn files.
.DE
.LP
The `janet' mailer will use \*(uN. JANET is the name of the UK Joint
Academic Network, but the mailer is equally valid for any network
handled by \*(uN.
The question about the version of Sendmail is there because later
versions feed
.I transport
addresses through the per-mailer rulesets as well as
.I header
addresses. If you don't understand that,
.B "don't panic"
because it does not really matter. Just answer the question, and if in
doubt say yes!
.LP
The MAKE script has now finished, and has issued a reminder that the
job is not yet finished.
At this point, it would be a good idea to look in the \*(uN
distribution, in the directory
.I sendmail
and select a mailer description to replace
.I hhcpm.m4
in the newly-created directory. You should check that the pathname of
the mailer binary (ni_send) is correct for your installation.
.LP
The next job is to make the domain files, so
.I "cd\ dom"
and read the README file.
You will probably want to extract the example `.dom' files and look at
them:
.DS
ar x Examples.a
.DE
You can probably ignore (and delete) the bitnet.dom and csnet.dom
files. The ukuucp.dom file may not be necessary either. Look at the
file ed.local.dom, and create something similar for your own site.
This maps from machine names to full domain names and from any
subdomains to full domain names, so Tring (which has
only two machines at present) would have a file
.I tring.local.dom
looking like this:
.DS
# tring.local.dom
#
# maps from machine names to full domain names

Suleika         Suleika.tring.ac.uk
Ceres           Ceres.tring.ac.uk
.DE
.LP
If the
.I tring.ac.uk
domain were to be split into subdomains, say to allow three
departments to manage their own mail, it might look like this:
.DS
# tring.local.dom
#

Suleika		Suleika.cs.tring.ac.uk
Ceres		Ceres.cs.tring.ac.uk
Juno		Juno.ee.tring.ac.uk
Papagena	Papagena.ee.tring.ac.uk
Astraea		Astraea.me.tring.ac.uk
.DE
.LP
That sort of trick is a little more complicated: refer to the
UK-sendmail documentation for full details. We will stick with
the simpler case in this document.
.LP
The
.I top.dom
file lists `top level domains' such as UK and NZ. The list supplied
with UK-sendmail is now rather out of date, but will do for now. At a
later stage you should obtain an up-to-date list, perhaps by mailing
the info-server at uk.ac.ukc (request: uknet; topic: top-domains).
If in doubt about what `relay site' to specify for any domain,
the best default option is ukc. One line in the supplied
.I top.dom
file
.B must
be changed if you have more than one host: The UK domain should be
relayed by whichever machine you are installing \*(uN on:
.DS
# top.dom
#  Top level domains
#  if address hasn't matched a domain name in any other table then
#  it should match something here. The RHS is a domain which specifies
#  where the message should be relayed for further routing.
#
uk		Suleika.tring.ac.uk
com		ukc.ac.uk
gov		ukc.ac.uk
edu		ukc.ac.uk
mil		ukc.ac.uk
bitnet		ukc.ac.uk
csnet		ukc.ac.uk
nl		ukc.ac.uk
au		ukc.ac.uk
nz		ukc.ac.uk
oz		ukc.ac.uk
.DE
.LP
The
.I uk.dom
table can be left exactly as it comes until you have the NRS processor
going. If you are not in the UK domain, there will probably be some
equivalent table to replace
.I uk.dom.
.LP
The domain tables can now be merged using the MAKE script in the
.I dom
directory:
.DS
MAKE tring tring.local.dom top.dom uk.dom
.DE
.LP
The channel tables now have to be made, so
.I "cd\ ../chn"
and look at the README file.
Extract the examples with
.I "ar\ x\ Examples.a"
and have a look at them. Again, you can delete the ones not required.
Edit the remaining
.I .chn
files to suit your local environment. Here are examples for Tring:
.DS
# ether.chn
#
# Ethernet channel table
#  The domain name on the LHS is routed to the host on the RHS.
#
Suleika.tring.ac.uk     Suleika
Ceres.tring.ac.uk       Ceres
.DE
.DS
# local.chn
#
#  The Local channel
#    The domain names listed here are local to this host
#
#       NRS names
#       Include both long and short forms 
tring.ac.uk 
#       UUCP name (if required)
# tring.uucp 
#
#  As we are a multihost site, we must include the following host
names
#
Suleika.tring.ac.uk
Ceres.tring.ac.uk
.DE
.LP
The
.I janet.chn
file can be left empty for now, and filled in by the NRS
processor. One point worth noting: if you are using X25 to connect one
or more of the machines in your domain, you will need to list them in
.I janet.chn
to force their mail into \*(uN.
UK-sendmail has a default rule that causes all mail to subdomains of
`UK' to be handed to the `janet' mailer. If you are not in the `UK'
domain, you may need to modify the rule or place a list of sites into
.I janet.chn.
.LP
The
.I top.chn
file may be linked to ../dom/top.dom for simplicity.
.LP
You are now ready to merge the channel tables:
.DS
MAKE Suleika local.chn ether.chn janet.chn top.chn
.DE
.LP
The hard work is now over;
.B "cd\ ../Suleika"
and type
.B make
(That is the normal, lower-case, make!)
Make will copy the domain and channel tables and merge them with the
machine-specific information set up in the first phase. You can then
test the configuration by typing:
.B "/usr/lib/sendmail\ -bt\ -CSuleika.cf"
You should test ruleset zero with a good selection of possible
addresses (both correct ones and bad ones). Ensure that all correct
addresses eventually produce a line specifying a mailer and an
address. 
Here are a couple of examples:
.DS
/usr/lib/sendmail -bt -CSuleika.cf
ADDRESS TEST MODE
Enter <ruleset> <address>
> 0 fred
rewrite: ruleset  3   input: "fred"
rewrite: ruleset  3 returns: "fred"
rewrite: ruleset  0   input: "fred"
rewrite: ruleset  0 returns: "^V" "local" "^X" "fred"
> 0 bill@tring
rewrite: ruleset  3   input: "bill" "@" "tring"
rewrite: ruleset 11   input: "bill" "@" "tring"
rewrite: ruleset 11 returns: "bill" "@" "tring" "." "ac" "." "uk"
and so on........
rewrite: ruleset 15 returns: "bill"
rewrite: ruleset  0 returns: "^V" "local" "^X" "bill"
.DE
.LP
Any off-site addresses should end up using the `hhcp' mailer (hhcp is
the name of a mailer used by an old X25 NIFTP package)
Addresses within your own domain should end up using the `local'
mailer.
.LP
Once you are happy with the behaviour of the configuration, install it
with:
.B "make\ install"
You should then find the existing sendmail daemon and kill it before
starting a new daemon with:
.DS
/usr/lib/sendmail -bd -q1h
.DE
or whatever command you find in your /etc/rc.local
.LP
The job is done! If you have other machines to configure, you should
make Sendmail configurations for them at some stage. These will
probably be much the same as the first one, but without the `janet'
mailer if they are not going to use \*(uN.
.NH 4
Testing the sendmail configuration
.LP
The address-testing phase above should catch most errors, but the only
way to test the whole system is to send a message. Start by mailing
something to yourself without specifying any domain names:
.DS
echo hello | mail fred
.DE
.LP
If that works, try your local domain name too:
.DS
echo hello | mail fred@tring.ac.uk
.DE
.LP
None of that should have gone anywhere near \*(uN, so the next thing
is to try a bouncer. Pick some friendly site with a reasonably smart
mailer (e.g. a site running MMDF or Sendmail) and bounce a message off
them:
.DS
echo bounce bounce bounce | mail fred%tring.ac.uk@brunel.ac.uk
.DE
.LP
That request should put a mail message into the \*(uN outgoing queue.
You can then use
.I ftpq
to track its progress (e.g. ftpq -mald), and look in the \*(uN logs to see
what happened to the message.
If everything is set up correctly and the remote site is alive, you
should get your message back within a few minutes.
If not, look in the ftpspool log (the argument to ftpspool in /etc/rc.local)
and the log files in (e.g.) /usr/spool/niftp/logs.
.LP
.NH 4
Likely problems
.IP "Illegal format JNT mail file."
This is an error message that may be sent by a remote MMDF site.
It means that the remote site cannot identify you from your address.
The solution is to get your X25 calling address correctly registered
in the NRS.
.IP "Unknown host uk.ac.xyzzy"
The host uk.ac.xyzzy is not in the database used by \*(uN.
Sendmail will pass the address to \*(uN quite happily because it seems
to be a valid UK address.
.NH 4
Disgusting HACK
.LP
If you are an academic site with a JANET line but no PSS line (or a
commercial site with PSS but not JANET) you will want some way of
relaying mail into the `other' domain. UKC will do this for you, as
will a number of other well-connected sites. The problem is to get the
mail to UKC without putting the name of every UK.CO site into
.I janet.chn.
The trick here is to depend on the `domain escalation' that is built
into ni_send. Where an address of the form 'uk.ac.xx.yy' is presented
to ni_send and not found in the database, the least-significant domain
is stripped off and the lookup is retried. Thus, mail for an unknown
domain `uk.ac.xx.yy' would be sent to a known domain `uk.ac.xx' for
forwarding. (This assumes that the machine representing that domain is
.I capable
of forwarding! Not many non-Unix machines can do it...). Taking this
one stage further, if there were a known site `uk.co', it could relay
all mail for subdomains of `uk.co'. There is no such site, but since
the mailer at UKC is quite tolerant it is possible to make a phantom
site
.I called
`uk.co' but having UKC's
.I "network address."
This is done with a line in the UAIEF file:
.DS
h:uk.co:uk.co:janet:4:000049200000:FTP.MAIL:::0:50:0:
.DE
.LP
For a commercial site on PSS wishing to have all `uk.ac' mail relayed,
the equivalent line would be:
.DS
h:uk.ac:uk.ac:pss:4:234222715151:FTP.MAIL:::0:50:0:
.DE
.LP
See the section on setting up the database for more information on
UAIEF.
.LP
Note however, that this is contrary to the GreyBook spec [[or is it the NRS]].
.br
