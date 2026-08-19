# Gate 5.3 B3I Generated-RTL Verification Scope

## Generated RTL Tops

The B3I helper testbench targets the canonical `synth_fetch_packet_top`. It verifies observable pure-helper behavior for two-lane packet construction, carry creation and completion, lane fault attribution, metadata, mask restrictions, absence of partial lanes, and deterministic repeated invocation.

The separate B3I integration testbench targets the canonical `synth_fetch_packet_frontend_top`. Through its real IMEM request and response streams it verifies packet admission and FIFO order, two-lane capacity behavior, pending stability, atomic rejection/admission, carry integration, reset and flush kills, branch redirect priority, response identity draining, and pending fault cancellation. These claims rely only on outputs exported by that canonical top.

The combined `rtl_test_matrix.csv` is created only after both simulations pass. Every row identifies either `fetch_packet_helper` or `fetch_packet_frontend` scope and is not pre-populated with unevidenced PASS rows.

## Integration Coverage

The helper does not claim integration behavior. The canonical frontend top now provides the following generated-RTL coverage:

| Requirement | Required verification boundary | Status |
|---|---|---|
| Fetch Buffer packet atomicity and backpressure | `synth_fetch_packet_frontend_top` | COVERED_BY_FRONTEND_RTL |
| Branch redirect priority and carry cancellation | `synth_fetch_packet_frontend_top` | COVERED_BY_FRONTEND_RTL |
| Stale IMEM response drain by fetch ID, epoch, and address | `synth_fetch_packet_frontend_top` | COVERED_BY_FRONTEND_RTL |

Odd-redirect instruction-address-misaligned subtype output is not exposed by `synth_fetch_packet_frontend_top`; no B3I generated-RTL matrix row claims that unavailable semantic.

## Intended Invocation

From the repository root:

```bash
HLS_BOOM_ROOT=/home/lab_726/boom/hls_boom bash scripts/gate5_3/run_b3i_packet_rtl.sh
```

This command generated both tops, ran both focused Vivado simulations, and wrote the scope-labelled matrix. Final result: helper 59/59 plus Frontend integration 36/36, total 95/95 PASS.
