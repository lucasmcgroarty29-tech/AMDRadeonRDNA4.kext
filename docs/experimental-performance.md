# Experimental performance tests

This document applies only to `tahoe-26.7.3-experimental`. The stable branch keeps its original 250 us active host polling cadence.

## Experiment 1: active host polling interval

The experimental branch accepts the boot argument:

```
rdna4-pvtick-us=250
rdna4-pvtick-us=500
rdna4-pvtick-us=750
rdna4-pvtick-us=1000
```

- Default: 250 us, matching stable.
- Accepted range: 250..1000 us.
- Values outside the range are ignored and fall back to 250 us.
- The idle interval remains 1 ms.
- The selected interval is logged at host startup.

### Hypothesis

A longer active interval may reduce repeated polling overhead and leave more CPU time for other work. It may also increase command, stamp and display-event latency. This is an A/B experiment, not an assumed optimisation.

### Test method

1. Keep the same macOS build, display refresh rate, resolution and workload.
2. Test 250 us first, then 500, 750 and 1000 us separately.
3. For each setting, repeat the same window open/close, minimise/restore and drag workload.
4. Compare host tick duration and lateness, FIFO/channel timing, VBL lateness, missed frames and visible animation smoothness.
5. Revert to 250 us if display events become less reliable, frames are missed, or interaction becomes less responsive.

Do not change multiple experimental variables in one test. The stable branch is unchanged and remains the fallback.

## Current status

Source change committed, but not yet compiled or tested on an RX 9070 XT. No performance improvement is claimed until hardware measurements confirm it.
