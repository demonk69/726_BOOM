# Gate 5.3 B2 Phase B Throughput Analysis

## Models

`b2_canonical_frontend_depth8` calls the product `boom::frontend_module` once per reported cycle. It drives only matched `ImemResponse` traffic from requests emitted by that frontend; responses arrive on the next call and carry the request address, fetch ID, and epoch. Words are deterministic legal 32-bit ADDI instructions. The harness never writes `producer_valid` or calls `fetch_buffer_step` directly. Decode readiness is represented at the canonical frontend boundary by its existing dispatch/decode valid inputs.

`gate5_2_unbuffered_model` is the explicit no-queue comparison: one IMEM request may be outstanding and its deterministic next-call response occupies one producer register. A Decode stall holds that register and prevents another request until acceptance. It has no occupancy or enqueue events. This is a reference model, not a second implementation of the B2 buffer.

Scenarios are A always ready, B one Decode stall every four cycles, C alternating ready/stalled (50%), D four stalled then sixteen ready cycles, and E eight stalled then twenty-four ready cycles. Each row ends after 2048 Decode acceptances.

## Results

| Scenario | Path | Instructions | Cycles | FE stalls | Full cycles | Max occ. | Enq | Deq | Decode stalls | Requests | Responses | Protocol constraints | IPC |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
|A_ALWAYS_READY|gate5_2_unbuffered_model|2048|2049|0|0|0|0|2048|0|2048|2048|0|0.999512|
|A_ALWAYS_READY|b2_canonical_frontend_depth8|2048|2051|0|0|1|2049|2048|0|2051|2050|0|0.998537|
|B_STALL_1_OF_4|gate5_2_unbuffered_model|2048|2731|682|0|0|0|2048|682|2048|2048|0|0.749908|
|B_STALL_1_OF_4|b2_canonical_frontend_depth8|2048|2734|676|2707|8|2056|2048|683|2058|2057|0|0.749086|
|C_STALL_50_PERCENT|gate5_2_unbuffered_model|2048|4097|2048|0|0|0|2048|2048|2048|2048|0|0.499878|
|C_STALL_50_PERCENT|b2_canonical_frontend_depth8|2048|4099|2041|4084|8|2056|2048|2049|2058|2057|0|0.499634|
|D_BURST_STALL_4|gate5_2_unbuffered_model|2048|2560|511|0|0|0|2048|512|2048|2048|0|0.800000|
|D_BURST_STALL_4|b2_canonical_frontend_depth8|2048|2560|502|2519|8|2056|2048|512|2058|2057|0|0.800000|
|E_BURST_STALL_8|gate5_2_unbuffered_model|2048|2736|687|0|0|0|2048|688|2048|2048|0|0.748538|
|E_BURST_STALL_8|b2_canonical_frontend_depth8|2048|2736|678|2703|8|2056|2048|688|2058|2057|0|0.748538|

## Executable Integrity Proof

The audit asserts every cycle that occupancy is at most eight and that canonical `FrontendState::stalled` capacity backpressure is impossible below full. Across the stalled portions of B-E it observed 2961 matched responses, 2961 resulting held/produced instructions, 30 enqueues below full, and 30 occupancy increases. It observed 3897 full-capacity backpressure cycles and zero below-full capacity backpressure cycles. Occupancies 1 through 8 were each reached while Decode was stalled. The separately reported protocol-constraint column counts no-progress bubbles attributable to the canonical one-outstanding-request boundary rather than queue capacity.

## Interpretation

The depth-eight queue does not improve long-run Decode-limited IPC: all paths must wait for the same readiness schedule. Its demonstrated benefit is decoupling: requests, responses, production, and enqueues continue during stalls while occupancy rises, and recurring stalls in B-E eventually reach capacity because their average Decode acceptance is below the frontend production rate. Request and enqueue counts can exceed 2048 because the canonical frontend remains ahead when the measurement stops; those are retained in the CSV rather than being relabeled as completed instructions. These are native C++ call-level results, not HLS wrapper clock or timing claims.
