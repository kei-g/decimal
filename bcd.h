#ifndef __include_bcd_h__
#define __include_bcd_h__

#ifdef _DEBUG
	#define BCD_DUMP_PARAMETERS const bcd_t *bcd, FILE *stream
	#define BCD_DUMP_PUT(v) putc('0' + (v), stream)
#else /* _DEBUG */
	#define BCD_DUMP_PARAMETERS const bcd_t *bcd
	#define BCD_DUMP_PUT(v) putchar('0' + (v))
#endif /* _DEBUG */

void bcd_dealloc(const dec_t *dec, bcd_t *bcd);
void bcd_dump(BCD_DUMP_PARAMETERS);
bool bcd_init(const dec_t *dec, bcd_t *bcd, uint8_t value);
void bcd_mul16(bcd_t *bcd);
void bcd_muladd(bcd_t *cur, const bcd_t *radix, uint8_t value);

#endif /* __include_bcd_h__ */
