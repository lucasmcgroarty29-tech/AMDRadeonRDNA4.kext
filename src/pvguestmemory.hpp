/*
 * pvguestmemory.hpp
 * AMDRadeonRDNA4
 *
 * Overflow-safe arithmetic for guest virtual-address ranges used by PVGPU.
 * Kept independent of IOKit so its boundary cases can be host-tested.
 */

#ifndef PvGuestMemory_hpp
#define PvGuestMemory_hpp

#include <stdint.h>

namespace PvGuestMemory
{

struct PageRange {
	uint64_t first;
	uint64_t count;
	uint64_t window;
	uint64_t windowOffset;
};

inline bool fitsDescriptor(uint64_t offset, uint64_t bytes, uint64_t length)
{
	return bytes && offset <= length && bytes <= length - offset;
}

inline bool makePageRange(uint64_t address, uint64_t bytes, PageRange &out)
{
	if (!bytes || bytes - 1 > UINT64_MAX - address)
		return false;

	const uint64_t last = address + bytes - 1;
	const uint64_t firstPage = address & ~0xfffull;
	const uint64_t lastPage = last & ~0xfffull;
	const uint64_t window = address & ~((2ull << 20) - 1);

	out = { firstPage, ((lastPage - firstPage) >> 12) + 1, window, address - window };
	return true;
}

inline bool fitsWindow(const PageRange &range, uint64_t bytes, uint64_t maxBytes)
{
	constexpr uint64_t kWindow = 2ull << 20;
	return bytes && bytes <= maxBytes && range.windowOffset < kWindow && bytes <= kWindow - range.windowOffset;
}

} // namespace PvGuestMemory

#endif // PvGuestMemory_hpp
