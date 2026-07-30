# Gate 3.10 Local Pipeline

Gate 3.10 uses the Gate 3.9 F1 reset baseline and real Vitis HLS schedule reports. Status is `LOCAL_PIPELINE_CHARACTERIZED_NO_ACCEPTED_CANDIDATE`.

The only legal loop experiment, `R1_RESET_INIT_PIPELINE`, achieves II=1 and passes XSim 49/49, but worsens reset latency by 198 RTL cycles and changes normal event cycles. All named normal-path candidates are excluded by real state recurrence, handshake, or same-cycle recovery semantics.

Requested 4.5 ns gives the best HLS estimate, 3.255 ns, but normal RTL cycles change. Gate 3.9 commit `557bdf5` remains accepted. M014 is `VERIFIED`; M009 remains `PARTIALLY_VERIFIED`; official Gate 3 remains false.
