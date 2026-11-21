#include "../src/pvrange.hpp"

#include <cstdio>
#include <cstdint>
#include <limits>

static unsigned failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); failures++; } } while (0)

int main()
{
	uint64_t value = 0;

	CHECK(pvrange::contains(4096, 0, 4096));
	CHECK(pvrange::contains(4096, 4096, 0));
	CHECK(!pvrange::contains(4096, 4097, 0));
	CHECK(!pvrange::contains(4096, 4095, 2));
	CHECK(pvrange::contains(UINT64_MAX, UINT64_MAX - 7, 7));
	CHECK(!pvrange::contains(UINT64_MAX, UINT64_MAX - 7, 8));

	CHECK(pvrange::addressRangeFits(0, 1));
	CHECK(pvrange::addressRangeFits(UINT64_MAX, 1));
	CHECK(pvrange::addressRangeFits(UINT64_MAX - 4095, 4096));
	CHECK(!pvrange::addressRangeFits(UINT64_MAX, 2));
	CHECK(!pvrange::addressRangeFits(0, 0));

	CHECK(pvrange::multiply(1920, 1080, value) && value == 2073600);
	CHECK(pvrange::multiply(0, UINT64_MAX, value) && value == 0);
	CHECK(!pvrange::multiply(UINT64_MAX, 2, value));
	CHECK(!pvrange::multiply(UINT64_MAX, UINT64_MAX, value));

	CHECK(pvrange::pageCount(0, 4096, 4096, value) && value == 1);
	CHECK(pvrange::pageCount(1, 4096, 4096, value) && value == 2);
	CHECK(pvrange::pageCount(4095, 1, 4096, value) && value == 1);
	CHECK(pvrange::pageCount(4095, 2, 4096, value) && value == 2);
	CHECK(pvrange::pageCount(4096, 4096, 4096, value) && value == 1);
	CHECK(pvrange::pageCount(UINT64_MAX - 4095, 4096, 4096, value) && value == 1);
	CHECK(pvrange::pageCount(UINT64_MAX - 4095, UINT64_MAX, 4096, value) && value > 1);
	CHECK(!pvrange::pageCount(0, 0, 4096, value));
	CHECK(!pvrange::pageCount(0, 1, 0, value));

	// Large page sizes must not fail just because span + tail would overflow.
	CHECK(pvrange::pageCount(UINT64_MAX - 1, 2, UINT64_MAX, value) && value == 2);
	CHECK(pvrange::pageCount(UINT64_MAX - 2, 2, UINT64_MAX, value) && value == 1);
	CHECK(pvrange::pageCount(0, UINT64_MAX, UINT64_MAX, value) && value == 1);

	// Exhaustively compare small ranges with a simple reference calculation.
	for (uint64_t page = 1; page <= 16; ++page) {
		for (uint64_t offset = 0; offset <= 31; ++offset) {
			for (uint64_t bytes = 1; bytes <= 64; ++bytes) {
				const uint64_t span = offset % page;
				const uint64_t expected = (span + bytes + page - 1) / page;
				CHECK(pvrange::pageCount(offset, bytes, page, value));
				CHECK(value == expected);
			}
		}
	}

	if (failures) {
		std::printf("%u range test(s) failed\n", failures);
		return 1;
	}
	std::puts("GPU memory range tests passed");
	return 0;
}
