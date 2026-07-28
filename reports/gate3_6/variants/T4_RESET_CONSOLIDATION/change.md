# T4_RESET_CONSOLIDATION

Status: `REJECTED_RESET_SEMANTICS`.

T3 removed the wrapper but left zero automatic partitions and increased direct state-memory LUT to 5011. Gate 3.6 therefore performs one diagnostic product-top synthesis with the `HLS RESET variable=state` pragma removed to isolate reset elaboration.

This diagnostic cannot be accepted as an architectural configuration because explicit hardware reset semantics are required. It is run only to classify the top-level resource delta; the accepted reset pragma will be restored afterward.

Outcome: 45602 LUT, 12119 FF, 12 BRAM_18K, 3 DSP, 5.898 ns, and 342 automatic partitions. The required reset pragma was restored after the measurement.
