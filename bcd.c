#include "decimal.h"

#include "assert.h"
#include "assume.h"
#include "bcd.h"

#define BCD_LOG10_16_SHL62 5553023288523357133UL

static void bcd_dump_full(const bcd_t *bcd);
static size_t bcd_length(const bcd_t *bcd);

void bcd_dealloc(const dec_t *dec, bcd_t *bcd)
{
	dec_munmap(dec, bcd->addr, bcd->capacity);
}

void bcd_dump(BCD_DUMP_PARAMETERS)
{
	size_t length = bcd_length(bcd);
	for (size_t i = 0; i < length; i++) {
		uint8_t v = bcd->head[i];
		if (i || v >> 4)
			BCD_DUMP_PUT(v >> 4);
		BCD_DUMP_PUT(v & 15);
	}
}

bool bcd_init(const dec_t *dec, bcd_t *bcd, uint8_t value)
{
	ASSERT(value >> 4 < 10);
	ASSERT((value & 15) < 10);
	__uint128_t size = dec->total;
	size *= BCD_LOG10_16_SHL62;
	size >>= 62;
#ifdef _DEBUG
	fprintf(stderr, "%zu bytes required to represent %zu bytes hex\n",
		(uint64_t)size, dec->total);
#endif /* _DEBUG */
	bcd->capacity = ((uint64_t)size + 0xfff) & ~0xfff;
	if ((bcd->addr = dec_mmap(dec, bcd->capacity))) {
		bcd->head = (uint8_t *)bcd->addr + bcd->capacity;
		*--bcd->head = value;
	}
	return !!bcd->addr;
}

void bcd_mul16(bcd_t *bcd)
{
	size_t length = bcd_length(bcd);
#ifdef _DEBUG
	bcd_dump(bcd, stderr);
	fprintf(stderr, "(%zu/%zu,addr=%p,head=%p) x 16 = ",
		length, bcd->capacity, bcd->addr, bcd->head);
#endif /* _DEBUG */
	uint8_t carry = 0;
	for (size_t i = 1; i <= length; i++) {
		uint8_t *const r = bcd->head + length - i;
		uint8_t rn[2] = { *r & 15, *r >> 4 };
		for (int j = 0; j < 2; j++) {
			ASSERT(rn[j] < 10);
			rn[j] *= 16;
			ASSERT(rn[j] <= 144); // <= 0x90
			ASSERT(carry <= 16);
			rn[j] += carry;
			ASSERT(rn[j] <= 160); // <= 0xa0
			carry = rn[j] / 10;
			ASSERT(carry <= 16);
			rn[j] %= 10;
		}
		ASSERT(rn[0] < 10 && rn[1] < 10);
		*r = rn[1] << 4 | rn[0];
	}
	ASSERT(carry <= 16);
	if (carry)
		*--bcd->head = (carry / 10) >> 4 | (carry % 10);
	bcd_dump_full(bcd);
}

void bcd_muladd(bcd_t *cur, const bcd_t *radix, uint8_t value)
{
	size_t radixlen = bcd_length(radix);
#ifdef _DEBUG
	bcd_dump(cur, stderr);
	size_t curlen = bcd_length(cur);
	fprintf(stderr, "(%zu/%zu,addr=%p,head=%p) + ",
		curlen, cur->capacity, cur->addr, cur->head);
	bcd_dump(radix, stderr);
	fprintf(stderr, "(%zu/%zu,addr=%p,head=%p) x %hhu = ",
		radixlen, radix->capacity, radix->addr, radix->head, value);
#endif /* _DEBUG */
	ASSERT(value < 16);
	size_t newlen = radixlen + 1;
	if (bcd_length(cur) < newlen)
		cur->head = (uint8_t *)cur->addr + cur->capacity - newlen + 1;
	size_t length = newlen - 1;
	uint8_t carry = 0;
	for (size_t i = 1; i <= radixlen; i++) {
		uint8_t r = radix->head[radixlen - i];
		uint8_t *const c = cur->head + length - i;
		uint8_t rn[2] = { r & 15, r >> 4 };
		uint8_t cn[2] = { *c & 15, *c >> 4 };
		for (int j = 0; j < 2; j++) {
			ASSERT(rn[j] < 10);
			rn[j] *= value;
			ASSERT(rn[j] <= 135); // <= 0x87
			ASSERT(cn[j] < 10);
			rn[j] += cn[j];
			ASSERT(rn[j] <= 144); // <= 0x90
			ASSERT(carry < 16);
			rn[j] += carry;
			ASSERT(rn[j] < 160); // < 0xa0
			carry = rn[j] / 10;
			ASSERT(carry < 16);
			rn[j] %= 10;
		}
		ASSERT(rn[0] < 10 && rn[1] < 10);
		*c = rn[1] << 4 | rn[0];
	}
	ASSERT(carry < 16);
	if (carry)
		*--cur->head = (carry / 10) << 4 | (carry % 10);
	for (; !*cur->head; cur->head++);
	bcd_dump_full(cur);
}

static void bcd_dump_full(const bcd_t *bcd)
{
#ifdef _DEBUG
	bcd_dump(bcd, stderr);
	size_t len = bcd_length(bcd);
	fprintf(stderr, "(%zu/%zu,addr=%p,head=%p)\n",
		len, bcd->capacity, bcd->addr, bcd->head);
#endif /* _DEBUG */
}

static size_t bcd_length(const bcd_t *bcd)
{
	ASSERT(bcd->head - (uint8_t *)bcd->addr <= bcd->capacity);
	return bcd->capacity - (bcd->head - (uint8_t *)bcd->addr);
}
