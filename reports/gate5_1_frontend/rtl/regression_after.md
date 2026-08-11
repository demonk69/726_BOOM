# Gate 5.1 Regression After

- Focused native Frontend: PASS.
- Focused UBSan Frontend: PASS.
- Gate 4.1 M3C directed: 1458 checks, PASS.
- Gate 4.1 M3C persistent random: 256 seeds x 2048 cycles, PASS; no arithmetic/protocol mismatch.
- RV64M native full-core: 15/15 PASS.
- Gate 4 W3 software baseline: FAIL, 399/400. `ROB fill` reports `rename packet was not retained by ROB backpressure` in `regression/w4/regression/w4e/product_full/logs/directed_tests.log`.
- W4 directed 95/95, W4 random 128/128, normalized traces, full-program 10/10, and partial-order 7/7: NOT_REACHED because the prerequisite W3 suite failed. Expected files were not changed.
- W4/W3 focused RTL rerun: TIMEOUT after 900 seconds during XSim elaboration; no acceptance matrix produced.
- RV64M csim/generated RTL 15/15, Gate 3.9 49/49, and full-core XSim 49/49: NOT_RUN after mandatory software regression failure and focused RTL blocker.

The accepted historical Gate 4.1 evidence remains the comparison baseline but cannot replace current-source reruns. Current-source full-core regression is not accepted.
