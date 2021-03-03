#include "decimal.h"

#include "assert.h"
#include "assume.h"
#include "bcd.h"

#include <sys/mman.h> /* for mmap, munmap */
#include <unistd.h>   /* for STDIN_FILENO, read */

#define DECIMAL_BLOCKSIZE (4ULL * 1024 * 1024)

static const char *basename(const char *argv0);

static void dec_dealloc(dec_t *dec);
static bool dec_main(dec_t *dec);
static bool dec_read(dec_t *dec);

static bool decblk_has_next_value(const decblk_t *blk, size_t i);
static void decblk_main(dec_t *dec, const decblk_t *blk);

int main(int argc, char *argv[])
{
	dec_t dec = { .progname = basename(argv[0]) };
	bool succeed = dec_main(&dec);
	if (succeed) {
#ifdef _DEBUG
		bcd_dump(&dec.current, stdout);
#else /* _DEBUG */
		bcd_dump(&dec.current);
#endif /* _DEBUG */
		bcd_dealloc(&dec, &dec.current);
		putchar('\n');
	}
	dec_dealloc(&dec);
	return succeed ? 0 : 1;
}

void *dec_mmap(const dec_t *dec, size_t size)
{
	const int prot = PROT_READ | PROT_WRITE;
	const int flags = MAP_ANONYMOUS | MAP_PRIVATE;
	void *addr = mmap(NULL, size, prot, flags, -1, 0);
	if (addr == MAP_FAILED)
		return DECIMAL_PERROR(dec, "mmap"), NULL;
	return addr;
}

bool dec_munmap(const dec_t *dec, void *addr, size_t size)
{
	if (munmap(addr, size) < 0)
		return DECIMAL_PERROR(dec, "munmap(%p)", addr), false;
	return true;
}

static const char *basename(const char *argv0)
{
	const char *last = argv0;
	for (const char *cur = argv0; *cur; cur++)
		if (*cur == '/')
			last = cur + 1;
	return last;
}

static void dec_dealloc(dec_t *dec)
{
	for (decblk_t *blk = dec->top; blk;) {
		decblk_t *next = blk->next;
		dec_munmap(dec, blk, DECIMAL_BLOCKSIZE);
		blk = next;
	}
}

static bool dec_main(dec_t *dec)
{
	if (!dec_read(dec) || !bcd_init(dec, &dec->radix, 1))
		return false;
	bool succeed = bcd_init(dec, &dec->current, 0);
	if (succeed)
		for (decblk_t *blk = dec->top; blk; blk = blk->next)
			decblk_main(dec, blk);
	bcd_dealloc(dec, &dec->radix);
	return succeed;
}

static bool dec_read(dec_t *dec)
{
	const size_t valsiz = DECIMAL_BLOCKSIZE - offsetof(decblk_t, values);
	for (;;) {
		decblk_t *blk = dec_mmap(dec, DECIMAL_BLOCKSIZE);
		if (!blk)
			return false;
		blk->length = 0;
		blk->next = dec->top;
		dec->top = blk;
		for (blk->length = 0; blk->length < valsiz;) {
			size_t len = valsiz - blk->length;
			void *buf = blk->values + blk->length;
			ssize_t rlen = read(STDIN_FILENO, buf, len);
			if (rlen < 0)
				return DECIMAL_PERROR(dec, "read"), false;
			if (rlen == 0)
				return true;
			ASSERT(0 < rlen);
			__assume(0 < rlen);
			blk->length += rlen;
			dec->total += rlen;
		}
	}
	__assume(0);
}

static bool decblk_has_next_value(const decblk_t *blk, size_t i)
{
	return i + 1 < blk->length || (blk->next && !blk->next->length);
}

static void decblk_main(dec_t *dec, const decblk_t *blk)
{
	for (size_t i = 0; i < blk->length; i++) {
		uint8_t v = blk->values[i];
		if (v & 15)
			bcd_muladd(&dec->current, &dec->radix, v & 15);
		if (v >> 4) {
			bcd_mul16(&dec->radix);
			bcd_muladd(&dec->current, &dec->radix, v >> 4);
		}
		if (!decblk_has_next_value(blk, i))
			break;
		bcd_mul16(&dec->radix);
		if (v >> 4 == 0)
			bcd_mul16(&dec->radix);
	}
}
