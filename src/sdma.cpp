//
//  sdma.cpp
//  AMDRadeonRDNA4
//
//  See sdma.hpp.
//

#include "sdma.hpp"

namespace Sdma {

uint32_t writeDword(uint32_t *out, uint64_t addr, uint32_t value) {
	if (!out || (addr & 3) || addr > UINT64_MAX - 3)
		return 0;
	out[0] = header(OpWrite, kSubLinear);
	out[1] = static_cast<uint32_t>(addr);
	out[2] = static_cast<uint32_t>(addr >> 32);
	out[3] = 0;                          // count: dwords - 1
	out[4] = value;
	return kWriteDwords;
}

uint32_t fence(uint32_t *out, uint64_t addr, uint32_t value) {
	if (!out || (addr & 3) || addr > UINT64_MAX - 3)
		return 0;
	out[0] = header(OpFence) | kFenceMtypeUc;
	out[1] = static_cast<uint32_t>(addr);
	out[2] = static_cast<uint32_t>(addr >> 32);
	out[3] = value;
	return kFenceDwords;
}

uint32_t trap(uint32_t *out) {
	if (!out)
		return 0;
	out[0] = header(OpTrap);
	out[1] = 0;                          // INT_CONTEXT(0)
	return kTrapDwords;
}

uint32_t constFill(uint32_t *out, uint64_t addr, uint32_t pattern, uint32_t bytes) {
	// SDMA 7 advertises a 4 MiB maximum fill; the count field stores bytes - 1.
	if (!out || !bytes || bytes > 0x400000 || (bytes & 3) || (addr & 3) ||
	    addr > UINT64_MAX - (static_cast<uint64_t>(bytes) - 1))
		return 0;
	out[0] = header(OpConstFill) | kFillSizeDword;
	out[1] = static_cast<uint32_t>(addr);
	out[2] = static_cast<uint32_t>(addr >> 32);
	out[3] = pattern;
	out[4] = bytes - 1;
	return kFillDwords;
}

uint32_t copyLinear(uint32_t *out, uint64_t src, uint64_t dst, uint32_t bytes) {
	// Keep individual packets within SDMA 7's 4 MiB copy limit.
	if (!out || !bytes || bytes > 0x400000 ||
	    src > UINT64_MAX - (static_cast<uint64_t>(bytes) - 1) ||
	    dst > UINT64_MAX - (static_cast<uint64_t>(bytes) - 1))
		return 0;
	out[0] = header(OpCopy, kSubLinear) | kCopyCpv;
	out[1] = bytes - 1;                  // count
	out[2] = 0;                          // parameter (swap modes)
	out[3] = static_cast<uint32_t>(src);
	out[4] = static_cast<uint32_t>(src >> 32);
	out[5] = static_cast<uint32_t>(dst);
	out[6] = static_cast<uint32_t>(dst >> 32);
	out[7] = 0;                          // DCC format: uncompressed
	return kCopyDwords;
}

uint32_t indirect(uint32_t *out, uint64_t buffer, uint32_t dwords, uint32_t vmid) {
	if (!out || !dwords || (dwords & 7) || vmid > 15 || (buffer & 0x1f) ||
	    buffer > UINT64_MAX - (static_cast<uint64_t>(dwords) * 4 - 1))
		return 0;
	out[0] = header(OpIndirect) | (vmid << 16);
	out[1] = static_cast<uint32_t>(buffer) & ~0x1fu;
	out[2] = static_cast<uint32_t>(buffer >> 32);
	out[3] = dwords;
	out[4] = 0;                          // no context-save area
	out[5] = 0;
	return kIndirectDwords;
}

bool Ring::init(volatile uint32_t *cpu, uint64_t mc, uint32_t sizeBytes, uint64_t startWptr) {
	if (!cpu || sizeBytes < 256 || (sizeBytes & (sizeBytes - 1)) || (mc & 0xff) ||
	    mc > UINT64_MAX - (static_cast<uint64_t>(sizeBytes) - 1) || (startWptr & 3))
		return false;
	ring = cpu;
	base = mc;
	size = sizeBytes;
	wp = startWptr;
	for (uint32_t i = 0; i < size / 4; i++)
		ring[i] = 0;                     // NOPs
	return true;
}

uint32_t Ring::sizeLog2Dwords() const {
	uint32_t n = 0;
	for (uint32_t v = size / 4; v > 1; v >>= 1)
		n++;
	return n;
}

bool Ring::emit(const uint32_t *dw, uint32_t count) {
	// Compare in dwords: count * 4 can wrap before the old byte-size check.
	// Keep one dword unused so a full ring is never confused with an empty one.
	if (!ring || !dw || !count || count >= size / 4 ||
	    wp > UINT64_MAX - static_cast<uint64_t>(count) * 4)
		return false;
	for (uint32_t i = 0; i < count; i++) {
		ring[(wp & (size - 1)) / 4] = dw[i];
		wp += 4;
	}
	return true;
}

} // namespace Sdma
