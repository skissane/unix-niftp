/* unix-niftp lib/mmdf/conf_niftp.c $Revision: 5.6 $ $Date: 1991/06/07 17:01:13 $ */
/*  NIFTP MMDF interface configuration file                     */
/*  This contains the configuration information for directories */
/*  Needed by the routines called by the NIFTP which interface  */
/*  Onto MMDF                                                   */
/*                                                              */
/*  These routines are compiled into the NIFTP to call the      */
/*  mailsystem at various points                                */
/*  Note that the strings are compiled in and NOT tailored      */
/*  Try not to pull in too much MMDF stuff into the NIFTP       */
/*                                                              */
/*      Steve Kille             August 1982                     */

/*
 *  Mar 83      Steve Kille   Change for new MMDF
 *                            Remove some mail dependencies from
 *                              the  NIFTP bits
 * $Log: conf_niftp.c,v $
 * Revision 5.6  1991/06/07  17:01:13  pb
 * Distribution of Oct90deslib+PPaids: Include des/lib/ and a few aids for PP sites
 *
 * Revision 5.5  90/08/01  13:36:06  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:48:25  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:45:09  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */


#ifndef ICDOC
char    nicdfldir[]     = "/usr/mmdf/lib";
				/* Default directory for NIFTP  */
				/* 'channel' code               */
#else
char    nicdfldir[]     = "/usr/local/mmdf";
				/* Default directory for NIFTP  */
				/* 'channel' code               */
#endif

char    nichan[]        = "ni_niftp";
				/* Routine called by the NIFTP   */
