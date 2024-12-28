#ifndef DES_PUBLIC
#define DES_PUBLIC 1
typedef unsigned char	des_u_char; /* This should be a 8-bit unsigned type */
typedef unsigned long	des_u_long; /* This should be a 32-bit unsigned type */

typedef struct {
  des_u_char	data[8];
} C_Block;

typedef struct {
  des_u_long	data[32];
} Key_schedule;

#define DES_ENCRYPT	0x0000
#define DES_DECRYPT	0x0001
#define DES_NOIPERM	0x0100
#define DES_NOFPERM	0x0200

des_u_long	des_cbc_cksum();

extern int	des_hash_inited;
extern Key_schedule	des_hash_key1;
extern Key_schedule	des_hash_key2;
extern /* const*/ C_Block	des_zero_block;

#define DES_HASH_INIT() (des_hash_inited ? 0 : des_hash_init())

extern char	*alo_getpass();
extern char	*alo_read_password();

#endif
