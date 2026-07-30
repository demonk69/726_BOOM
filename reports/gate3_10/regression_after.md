# Gate 3.10 Regression After

R1 passes all source-level regressions, frozen C++/csim traces, full-program architectural diff, partial-order inheritance, and XSim 49/49. It is rejected because reset latency worsens and normal RTL event cycles change.

No normal-path loop variant is run because every named L1-L5 loop has a real state recurrence, external handshake, or same-cycle recovery constraint. The accepted source configuration therefore remains the default macro-off Gate 3.9 behavior.
