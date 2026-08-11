# Gate 5.1 Throughput Analysis

The generated `synth_frontend_top` reports latency 2, interval 3, and no pipeline. A 32-instruction sequential generated-RTL run with the earliest compliant response/next-invocation schedule measured a steady request interval of 6 `ap_clk` cycles. Matching responses were accepted one cycle after the corresponding request in this focused driver, and the next request appeared five cycles after response acceptance.

One instruction requires two architectural wrapper invocations: one accepts/publishes the matching response, and a later invocation consumes/releases the one-entry handoff and emits the next request. Since the non-pipelined wrapper can start an invocation only every three clocks, the lower bound is `2 x 3 = 6` clocks per sequential instruction/request. The instruction publication interval is likewise 6 clocks with Decode modeled always-ready at the exposed boundary.

Thus HLS `Interval=3` is not itself the architectural request interval. It is the minimum top transaction initiation interval; the current two-invocation frontend protocol turns it into a six-clock request interval. This is worse than an unqualified one-request-per-cycle interpretation and no frozen acceptance exception authorizes it.

Redirect, Decode-stall, random-latency, and true zero-latency combinational-response throughput cannot be measured through the current focused top because their controls/observables are absent. Delayed response and request backpressure were measured and did not duplicate requests.

`GATE5_1_THROUGHPUT_BLOCKER=true`.
