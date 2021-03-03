#ifndef __include_decimal_h__
#define __include_decimal_h__

#include <errno.h>   /* for errno */
#include <stdbool.h> /* for bool */
#include <stddef.h>  /* for offsetof, size_t */
#include <stdint.h>  /* for uint8_t, uintmax_t */
#include <stdio.h>   /* for fprintf, stderr */
#include <string.h>  /* for strerror */

#ifdef __linux__
	#define DECIMAL_PERROR(dec, fmt, ...) \
		fprintf(stderr, "%s: " fmt ": %m\n", \
			dec->progname, ##__VA_ARGS__)
#else /* __linux__ */
	#define DECIMAL_PERROR(dec, fmt, ...) \
		fprintf(stderr, "%s: " fmt ": %s\n", \
			dec->progname, ##__VA_ARGS__, strerror(errno))
#endif /* __linux__ */

typedef struct _bcd bcd_t;
typedef struct _decimal_block decblk_t;
typedef struct _decimal dec_t;

void *dec_mmap(const dec_t *dec, size_t size);
bool dec_munmap(const dec_t *dec, void *addr, size_t size);

struct _bcd {
	void *addr;
	size_t capacity;
	uint8_t *head;
};

struct _decimal_block {
	size_t length;
	decblk_t *next;
	uint8_t values[];
};

struct _decimal {
	bcd_t current;
	const char *progname;
	bcd_t radix;
	decblk_t *top;
	uintmax_t total;
};

#endif /* __include_decimal_h__ */
