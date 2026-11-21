#ifndef RDNA4_PVRANGE_HPP
#define RDNA4_PVRANGE_HPP

#include <stdint.h>

namespace pvrange {

// Whether [offset, offset + bytes) fits in a region of length bytes, without adding the endpoints.
static inline bool contains(uint64_t length, uint64_t offset, uint64_t bytes)
{
	return offset <= length && bytes <= length - offset;
}

// Whether a non-empty address range has a representable inclusive final byte.
static inline bool addressRangeFits(uint64_t address, uint64_t bytes)
{
	return bytes && address <= UINT64_MAX - (bytes - 1);
}

// Checked product for dimensions and row sizes supplied by a memory descriptor.
static inline bool multiply(uint64_t a, uint64_t b, uint64_t &result)
{
	if (a && b > UINT64_MAX / a)
		return false;
	result = a * b;
	return true;
}

// Number of pages intersected by [offset, offset + bytes), including an unaligned first page.
// The split quotient/remainder calculation avoids overflowing offset-within-page + bytes.
static inline bool pageCount(uint64_t offset, uint64_t bytes, uint64_t pageSize, uint64_t &count)
{
	if (!bytes || !pageSize)
		return false;

	const uint64_t whole = bytes / pageSize;
	const uint64_t tail = bytes % pageSize;
	const uint64_t span = offset % pageSize;

	// Compute ceil((span + tail) / pageSize) without forming span + tail:
	// both operands can be close to UINT64_MAX for a very large page size.
	uint64_t extra = 0;
	if (tail) {
		extra = 1;
		if (span > pageSize - tail)
			++extra;
	} else if (span) {
		extra = 1;
	}
	if (whole > UINT64_MAX - extra)
		return false;

	count = whole + extra;
	return count != 0;
}

} // namespace pvrange

#endif
