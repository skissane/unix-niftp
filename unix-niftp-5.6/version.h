#ifdef	lint	/* unix-niftp version.h */
static char RCSid[]="$Header: /Nfs/blyth/glob/src/usr.lib/niftp/unix-niftp-5.6/RCS/version.h,v 5.6.1.10 1993/05/17 13:16:51 pb Rel $";
#endif	lint

/*
 * Overall version log
 *
 * patches should update this file together with other files effected.
 * the Prereq feature of patch should site the version of this files so
 * we all know where we are.
 * Official patches will be posted on the unix-niftp@cs.nott.ac.uk
 * mailinglist. If you maintain this software then send a message to
 * unix-niftp-request@cs.nott.ac.uk to joint the list.
 *
 * $Log: version.h,v $
 * Revision 5.6.1.10  1993/05/17  13:16:51  pb
 * Distribution of May1793FixVMSStreamCRLF: Re-do Format negotiation to avoid 0x80 VMS xfers with CRLF
 *
 * Revision 5.6.1.9  1993/05/10  14:05:01  pb
 * Distribution of May93FixSetgroups:   Ensure setgroups and setuid are more likley to work
 *
 * Revision 5.6.1.8  1993/05/10  14:05:01  pb
 * Distribution of May93FullSizeReadWithX25Header: Full Size read with X25_HEADER enabled discarded a byte
 *
 * Revision 5.6.1.7  1993/05/10  14:05:01  pb
 * Distribution of Apr93SunybytsdPPLDYbAANSICC:   Sun YBTSD + PP LD_ + YuckBucked ANSI CC preliminary HACK
 *
 * Revision 5.6.1.6  1993/01/10  07:07:28  pb
 * Distribution of Jan93FixMultiGroups: Fix multiple group access
 *
 * Revision 5.6  1990/10/21  07:43:59  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:29:24  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.4  89/08/26  13:42:29  pb
 * Distribution of Aug89PPsupport: Update READMEs for PP
 * 
 * Revision 5.3  89/07/16  14:29:03  pb
 * Distribution of Jul89PPsupport: Support PP spooled P and Q and unspooled Q
 * 
 * Revision 5.2  89/01/13  16:04:51  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.1  88/10/07  17:31:12  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.6  88/02/11  06:53:38  pb
 * Distribution of Jan88ReleaseMod2: More documentation + x25b + sendmail fixes + sun fixes
 * 
 * Revision 5.0.1.5  88/01/29  07:45:05  pb
 * Distribution of Jan88ReleaseMod1: JRP fixes- tcccomm.c ftp.c + news sucking rsft.c + makefiles
 * 
 * Revision 5.0.1.4  88/01/28  06:56:21  pb
 * Distribution of Jan88Release: Sun fixes plus adding x25b code
 * 
 * Revision 5.0.1.3  87/12/09  16:15:11  pb
 * Distribution of PreUKNETdecMeeting: Stable version before UKNET Dec 87 meeting
 * 
 * Revision 5.0.1.2  87/09/28  12:28:48  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0.1.1  87/06/01  07:28:38  pb
 * Reformat ...
 * 
 * Revision 5.0  87/03/23  03:12:27  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
#define VERSION "UNIX-NIFTP $Revision: 5.6.1.10 $"
