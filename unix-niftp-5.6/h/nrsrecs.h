/* unix-niftp h/nrsrecs.h $Revision: 5.5 $ $Date: 90/08/01 13:33:21 $ */
/*
 * $Log:	nrsrecs.h,v $
 * Revision 5.5  90/08/01  13:33:21  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * Revision 5.2  89/01/13  14:41:27  pb
 * Distribution of Jan89SuckMail: Support Sucking of mail
 * 
 * Revision 5.0  87/03/23  03:26:00  bin
 * Merger of latest UCL and Nottingham versions together with
 * an extensive spring clean.
 * Now UNIX-NIFTP prerelease.
 * 
 */
typedef struct  file_descriptor {
	FILE    *f_fp;
	int     f_length;       /* number of lines in the file */
	int     f_line;         /* current line number */
	long    f_offset;       /* offset into file of start of first line */
	unsigned *f_loffs;      /* indexes for the lines */
} FDS;

/*
 * Format of string used for each section
 */

#define FNAMEFORMAT             "tmp/section%02d"

#define TOFVAL(context, network)    ((context) * 256 + (network))

struct  f1rec   {
	int     m1_context;
	int     m1_network;
	int     m1_spare;
	int     m1_file;
	int     m1_offset;
};

struct  f2rec   {
	int     m4_off;
	int     m2_otheroff;
	int     m_namelong;
	int     m2_myoffset;
	char    *m_name;
};

struct  f3rec   {
	int     m3_context;
	int     m3_m2offset;
	char    *m3_addr;
};

#define MAXQUADS        30      /* enough for now */

struct  f4rec   {
	int     m4_count;
	struct  f4quads {
		int     m_fnumb;
		int     m_gways;
		int     m_off_indblock;
		int     m_nlines;
	} quads[MAXQUADS];
};

/*
 * structure of domain entry info
 */

#define MAXFDCNTS       10

struct  fdrec   {
	char    *fd_name;
	char    *fd_host;
	int     fd_count;
	int     fd_cnts[MAXFDCNTS];
};
