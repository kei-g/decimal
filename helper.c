#include <math.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	const long double rhs = log10l(16);
	uint64_t x = 5;
	for (int d = 2; d < 64; d++) {
		char fmt[32];
		sprintf(fmt, "%%20zu >> %%d = %%.%dLf", d);
		long double lhs = (long double)x / (1UL << d);
		printf(fmt, x, d, lhs);
		x <<= 1;
		if (lhs < rhs) {
			x++;
			printf(" is %.8Lg less than log10(16)\n", rhs - lhs);
		}
		else if (rhs < lhs) {
			x--;
			printf(" is %.8Lg greater than log10(16)\n", lhs - rhs);
		}
		else
			printf(" is equal to log10(16)");
	}
	return 0;
}
