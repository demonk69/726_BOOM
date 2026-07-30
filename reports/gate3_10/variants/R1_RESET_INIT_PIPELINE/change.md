# R1 Reset Init Pipeline Change

Classification: `RESET_ONLY`.

Only the 32-entry ROB valid/busy reset sweep is converted from one index per reset-controller call into a bounded local loop with `#pragma HLS PIPELINE II=1`. The variant is enabled only by `BOOM_HLS_GATE3_10_R1_RESET_ROB_PIPELINE`; the default source configuration retains the Gate 3.9 behavior.

C++ reset-controller calls fall from 145 to 114. No payload RAM is bulk-cleared, normal-path helper is changed, or `CORE_CYCLE` pipeline is enabled.
