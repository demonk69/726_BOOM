# PF1 Critical Path Analysis

The final `boom_core_top` HLS estimate is 6.341 ns against a 10.00 ns target,
unchanged from Gate 5.3. `synth_core_step_top` is also 6.341 ns and
`synth_frontend_top` is 6.071 ns. `CORE_CYCLE` remains unpipelined.

PF1 increases full-core LUT from 135,953 to 171,540 (+35,587, +26.18%) and FF
from 33,373 to 33,704 (+331, +0.99%). BRAM and DSP remain 16 and 3. The large
LUT delta is consistent with rebuilding speculative structures and rename
reachability in the exception recovery path; this is an HLS estimate, not a
post-route result. The zero-resource focused wrapper is constant-folded and is
used only as generated-RTL observability evidence, not as an overhead claim.
