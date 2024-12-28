/* unix-niftp lib/mmdf/conf_niftp.h $Revision: 5.5 $ $Date: 90/08/01 13:36:09 $ */
/*  Configuration file for NIFTP to MMDF code                           */
/*                                                                      */
/*  This interface is designed to look like an MMDF channel             */
/*  However it communicates between the NIFTP and Submit only           */
/*  It cannot be invoked by deliver.                                    */
/*                                                                      */
/*  Each NIFTP queue has a structure associated with it                 */
/*  This allows NIFTP queues to be mapped onto NIFTP channels           */
/*  Each NIFTP queue is mapped onto an NIFTP channel (not necessarily   */
/*  A one to one mapping).  This allows host verifiaction and           */
/*  other good things                                                   */
/*
 * $Log:	conf_niftp.h,v $
 * Revision 5.5  90/08/01  13:36:09  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:48:27  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:45:13  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
struct ni_chstruct {
	char *ni_spec;          /* Name of NIFTP channel for calling it by */
	char *ni_path;          /* Path to NIFTP channel                   */
	char *mm_chname;        /* Name of associated MMDF channel         */
	};


typedef struct ni_chstruct      Nichan;                 /* type as well */
