/*
 * pvscanout.hpp
 * AMDRadeonRDNA4
 *
 * The real heads as the paravirtual GPU's displays (PVGPU builds,
 * rdna4-pvgpu-disp bit 2): WindowServer composites them with Metal and
 * presents through the paravirtual display pipes, which only IOPresentment
 * drives, and IOPresentment takes a display pipe for every display
 * (docs/m0-pvgpu.md). So in such a boot the framebuffers report no display
 * and macOS sees only the pipes; each pipe's frames, SubmitTransaction's
 * IOSurface, are copied into its head's scanout surface.
 *
 * Port 0 is the boot head, ports 1.. the extra heads (rdna4-heads).
 */

#ifndef PvScanout_hpp
#define PvScanout_hpp

#include <stddef.h>
#include <stdint.h>

namespace PvScanout
{

/*
 * A validated, clipped IOSurface-to-scanout copy. Kept inline and free of
 * IOKit dependencies so the exact bounds logic used by the kernel path can
 * be exercised by the host regression tests.
 */
struct CopyLayout {
	uint32_t width, height;
	uint64_t rowBytes;
};

// The GPU-side presenter maps a complete set of source rows. Validate the
// extent with division before multiplying, so neither pitch * rows nor
// base + extent can wrap around the descriptor length.
inline bool makeSurfaceRange(uint64_t base, uint64_t length, uint64_t pitch, uint32_t rows, uint64_t &bytes) {
	if (!pitch || !rows || base > length)
		return false;
	const uint64_t remaining = length - base;
	if (rows > remaining / pitch)
		return false;
	bytes = pitch * rows;
	return true;
}

inline bool makeCopyLayout(uint64_t base, uint64_t length, uint64_t sourcePitch, uint32_t sourceWidth, uint32_t sourceHeight,
                           uint32_t destinationPitch, uint32_t width, uint32_t height, CopyLayout &out) {
	if (!sourcePitch || !sourceWidth || !destinationPitch || !sourceHeight || !width || !height)
		return false;
	const uint64_t pitchWidth = sourcePitch / 4;
	const uint64_t destinationWidth = destinationPitch / 4;
	if (width > sourceWidth)
		width = sourceWidth;
	if (width > pitchWidth)
		width = static_cast<uint32_t>(pitchWidth);
	if (width > destinationWidth)
		width = static_cast<uint32_t>(destinationWidth);
	if (height > sourceHeight)
		height = sourceHeight;
	const uint64_t rowBytes = static_cast<uint64_t>(width) * 4;
	if (!width || !height || rowBytes > destinationPitch || base > length || rowBytes > length - base)
		return false;
	if (height - 1 > (length - base - rowBytes) / sourcePitch)
		return false;
	out = { width, height, rowBytes };
	return true;
}

/* A head's scanout surface: 32 bits per pixel. */
struct Head {
	uint64_t phys;          /* physical address (the VRAM aperture) */
	uint32_t pitch;         /* bytes per row */
	uint32_t width, height;
};

/* Compare the defined fields, never ABI padding that may be indeterminate. */
inline bool sameHead(const Head &a, const Head &b) {
	return a.phys == b.phys && a.pitch == b.pitch && a.width == b.width && a.height == b.height;
}

/* At plugin start: debug.rdna4_pvheads, -1 until decided() (rdna4-iop switches IOPresentment by it). */
void start();

/* Whether the heads are the paravirtual displays: told once the boot head's framebuffer is attached. */
void decided(bool on);

/* Whether the heads are the paravirtual displays this boot (plugin.cpp). */
bool enabled();

/* Whether decided() has been told yet. */
bool known();

/*
 * The heads with a display (plugin.cpp): the boot head and the extra heads
 * that are lit or wait to be. Fixed at the first call, which numbers the
 * ports: the paravirtual GPU has one display a port.
 */
uint32_t count();

/* The mode head `port` is lit in, or will be once it can (plugin.cpp). */
bool mode(uint32_t port, uint32_t &width, uint32_t &height);

/* The EDID of what is on head `port` (plugin.cpp); false when there is none. */
bool edid(uint32_t port, const uint8_t *&data, size_t &len);

/* A mode a port's paravirtual display is offered: its size and refresh. */
struct Mode {
	uint32_t width, height, milliHz;
};

/*
 * The modes head `port`'s paravirtual display is offered (plugin.cpp): those
 * of its monitor's EDID the head's pipe can run, each size once at the
 * refresh nearest 60 Hz, the one it is lit in first; 0 when the head has
 * only the one it is in.
 */
uint32_t modes(uint32_t port, Mode *out, uint32_t cap);

/*
 * The largest mode head `port`'s pipe runs (plugin.cpp): what a frame larger
 * than any of them, of a scaled mode, is scaled down to.
 */
bool largest(uint32_t port, uint32_t &width, uint32_t &height);

/*
 * Head `port` switched to its mode of `width` x `height` (plugin.cpp): what
 * macOS presents in after the mode was chosen in the Displays settings.
 */
bool setMode(uint32_t port, uint32_t width, uint32_t height);

/*
 * Whether head `port` runs another mode of `width` x `height` than the one
 * macOS was given for that size (plugin.cpp): the boot display the GOP lit at
 * 3840x2160@30 while macOS was told 60 Hz. setMode then switches it.
 */
bool wantsMode(uint32_t port, uint32_t width, uint32_t height);

/* The scanout of head `port` (plugin.cpp); false while it is not lit. */
bool head(uint32_t port, Head &out);

/*
 * SubmitTransaction {u32 port, u32 surface, u32 task}: the surface, an
 * IOSurface ID, into head `port`'s scanout (pvscanout.cpp).
 */
bool present(uint32_t port, uint32_t surface, uint32_t task);

/*
 * At most `height` rows of at most `width` 4-byte pixels of IOSurface `sid`
 * into `dst`, `pitch` bytes a row: as much as both have (the host,
 * pvgpu.cpp).
 */
bool copySurface(uint32_t sid, uint8_t *dst, uint32_t pitch, uint32_t width, uint32_t height);

} /* namespace PvScanout */

#endif /* PvScanout_hpp */
