/* unix-niftp h/files.h $Revision: 5.5 $ $Date: 90/08/01 13:33:03 $ */
/*
 *      files.h
 *
 * last changed 9-May-85
 *
 *  file and process names used by ftp and its interfaces
 *
 * $Log:	files.h,v $
 * Revision 5.5  90/08/01  13:33:03  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.1  88/10/07  17:13:31  pb
 * Distribution of Oct88MultipleP: Support multiple P processes + window/pkt sizes + YBTS + banned
 * 
 * Revision 5.0.1.2  87/09/28  13:06:30  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.2  87/09/28  12:21:01  pb
 * Distribution of PreWjaTsSort: Fixes up to wja interlock to sort out common TS library
 * 
 * Revision 5.0  87/03/23  03:25:37  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
*/

#define QLOG            "log.q"
#define QSTT            "ST.q"
#define PLOG            "log.p"
#define PSTT            "ST.p"
#define QPROC           "qproc"
#define PPROC           "pproc"
#define DJTMP           "djtmp"
#define PSTAT           "stat.p"

#define LOCALLPDIR      "/tmp"
#define LOCALJOBDIR     "/tmp"
