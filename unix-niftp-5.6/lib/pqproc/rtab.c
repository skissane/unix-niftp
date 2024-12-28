/* unix-niftp lib/pqproc/timer.c $Revision: 5.5 $ $Date: 90/08/01 13:37:28 $ */
#include  "ntab.h"

/* file:  timer.c
 * last changed: 15-aug-83
 * $Log:	rtab.c,v $
 * Revision 5.5  90/08/01  13:37:28  pb
 * Distribution of Aug90RealPP+sequent: Full PP release and support for Sequent X.25 board
 * 
 * 
*/

/*
 * routine to read a tab structure & convert if necc ...
 */

union {
#ifdef	TAB0
	struct tab0 tab0;
#endif	TAB0
#ifdef	TAB1
	struct tab1 tab1;
#endif	TAB1
	char	dummy[sizeof(tab) + sizeof(long)];
} large;

#define	copy_val(type, field) tp->field = large.type.field;
#define	copy_var(type, field) strcpy(bufp, large.type.field); while(*bufp++);
#define	copy_fix(type, field)	bcopy(large.type.field, tp->field, sizeof large.type.field);

read_tab(fd, tp)
struct tab *tp;
{
	char *bufp;
	int n = read(fd, &large, sizeof large);

	switch (n)
	{
#ifdef	TAB0
	case	TAB0LEN:
#if	(TABV == 0)
		*tp = large.tab0;
		return TAB0LEN;
#elif	(TABV == 1)
		bufp = tp->text;
		tp->t_version		= TABV;
		tp->t_tlen		= TAB1LEN;

		copy_val(tab0, t_access);
		copy_val(tab0, t_flags);
		copy_val(tab0, bin_size);
		copy_val(tab0, status);
		copy_val(tab0, l_usr_id);
		copy_val(tab0, l_grp_id);
		copy_val(tab0, l_nretries);
		copy_val(tab0, l_nextattmpt);
		copy_var(tab0, t_queued);
		copy_var(tab0, l_network);
		copy_var(tab0, l_hname);
		copy_var(tab0, l_jtmpname);
		copy_var(tab0, l_from);
		copy_var(tab0, r_usr_n);
		copy_var(tab0, r_usr_p);
		copy_var(tab0, r_fil_n);
		copy_var(tab0, r_fil_p);
		copy_var(tab0, l_fil_n);
		copy_var(tab0, account);
		copy_var(tab0, acc_pass);
		copy_var(tab0, dev_type);
		copy_var(tab0, dev_tqual);
		copy_var(tab0, op_mess);
		copy_var(tab0, mon_mes);
		copy_var(tab0, specopts);
		copy_val(tab0, l_docet);

		copy_val(tab0, udocet.last_rlen);
		copy_val(tab0, udocet.st_of_tran);
		copy_val(tab0, udocet.last_count);
		copy_val(tab0, udocet.last_mark);
		copy_val(tab0, udocet.rec_mark);
		copy_val(tab0, udocet.lr_reclen);
		copy_val(tab0, udocet.lr_bcount);
		copy_fix(tab0, udocet.doc_p);
		copy_val(tab0, udocet.uid);
		copy_val(tab0, udocet.gid);
		copy_val(tab0, udocet.transfer_id);
		copy_fix(tab0, udocet.hname);
		copy_fix(tab0, udocet.tname);
		copy_fix(tab0, udocet.rname);

		tp->tptr		= bufp - (char *)tp;

		return TAB1LEN;
#else	/* TABV == ? */
		....
#endif	/* TABV == ? */
#endif	/* TAB0 */

#ifdef	TAB1
	case	TAB1LEN:
#if	(TABV == 0)
		copy_val(tab1, t_access);
		copy_val(tab1, t_flags);
		copy_val(tab1, bin_size);
		copy_val(tab1, status);
		copy_val(tab1, l_usr_id);
		copy_val(tab1, l_grp_id);
		copy_val(tab1, l_nretries);
		copy_val(tab1, l_nextattmpt);
		copy_var(tab1, t_queued);
		copy_var(tab1, l_network);
		copy_var(tab1, l_hname);
		copy_var(tab1, l_jtmpname);
		copy_var(tab1, l_from);
		copy_var(tab1, r_usr_n);
		copy_var(tab1, r_usr_p);
		copy_var(tab1, r_fil_n);
		copy_var(tab1, r_fil_p);
		copy_var(tab1, l_fil_n);
		copy_var(tab1, account);
		copy_var(tab1, acc_pass);
		copy_var(tab1, dev_type);
		copy_var(tab1, dev_tqual);
		copy_var(tab1, op_mess);
		copy_var(tab1, mon_mes);
		copy_var(tab1, specopts);
		copy_val(tab1, l_docet);

		copy_val(tab1, udocet.last_rlen);
		copy_val(tab1, udocet.st_of_tran);
		copy_val(tab1, udocet.last_count);
		copy_val(tab1, udocet.last_mark);
		copy_val(tab1, udocet.rec_mark);
		copy_val(tab1, udocet.lr_reclen);
		copy_val(tab1, udocet.lr_bcount);
		copy_fix(tab1, udocet.doc_p);
		copy_val(tab1, udocet.uid);
		copy_val(tab1, udocet.gid);
		copy_val(tab1, udocet.transfer_id);
		copy_fix(tab1, udocet.hname);
		copy_fix(tab1, udocet.tname);
		copy_fix(tab1, udocet.rname);

		tp->tptr		= bufp - tp;
		return TAB0LEN;
#elif	(TABV == 1)
		*tp = large.tab1;
		return TAB1LEN;
#else	/* TABV == ? */
		....
#endif	/* TABV == ? */
#endif	/* TAB1 */
	}
/*	L_WARN_4(L_GENERAL, 0,*/
	fprintf(stderr,
		"Tried to read a tab file of length %d: %08x %08x %08x\n",
		n, ((long *)(&large))[0],
		((long *)(&large))[1], ((long *)(&large))[2]);
	return 0;
}

